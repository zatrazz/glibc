/* Correctly-rounded reciprocal square root of binary64 value.

Copyright (c) 2022-2026 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/rsqrt/rsqrt.c, revision 304b681a).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <fenv.h>
#include <get-rounding-mode.h>
#include <libm-alias-double.h>
#include <math.h>
#include <stdint.h>
#include "math_config.h"
#include <math_uint128.h>

static __attribute__ ((noinline)) double
as_rsqrt_refine (double rf, double a)
{
  uint64_t ir = asuint64 (rf), ia = asuint64 (a);
  if (ia < UINT64_C (1) << MANTISSA_WIDTH)
    {
      /* Normalize a subnormal A.  Only the parity of the exponent is used
	 below.  */
      int nz = stdc_leading_zeros (ia);
      ia <<= nz - EXPONENT_WIDTH;
      ia &= MANTISSA_MASK;
      ia |= (uint64_t) (nz - 12) << MANTISSA_WIDTH;
    }
  /* An even power of 2 gives an exact result.  */
  if ((ia << EXPONENT_WIDTH) == UINT64_C (1) << 63)
    return rf;

  int mode = get_rounding_mode ();
  int e = (ia >> MANTISSA_WIDTH) & 1;
  uint64_t rm = (ir << EXPONENT_WIDTH | UINT64_C (1) << 63) >> EXPONENT_WIDTH;
  uint64_t am = (get_mantissa (ia) | UINT64_C (1) << MANTISSA_WIDTH)
		<< (5 - e);
  u128 rt = u128_mul (u128_from_u64 (rm), u128_from_u64 (am));
  uint64_t rth = u128_high (rt), rtl = u128_low (rt);
  u128 rrt = u128_mul (u128_from_u64 (rtl), u128_from_u64 (rm));
  uint64_t t0 = u128_low (rrt), t1 = u128_high (rrt) + rth * rm;
  rrt = u128_from_hl (t1, t0);
  int64_t s = u128_high (rrt) >> 63, dd = 1 - 2 * s;
  /* rts = s ? -(rt << 1) : rt << 1.  */
  u128 rt2 = u128_lshift (rt, 1);
  uint64_t ms = -(uint64_t) s;
  u128 rts = u128_from_hl (u128_high (rt2) ^ ms, u128_low (rt2) ^ ms);
  rts = u128_add (rts, u128_from_u64 (s));
  u128 prrt;
  uint64_t am2 = am << 1, am20 = -am;
  do
    {
      ir -= dd;
      prrt = rrt;
      am20 += am2;
      u128 tt = u128_sub (rts, u128_from_u64 (am20));
      rrt = u128_sub (rrt, tt);
    }
  while (__glibc_unlikely (!((u128_high (prrt) ^ u128_high (rrt)) >> 63)));
  if (!(u128_high (rrt) >> 63))
    {
      ir += dd;
      rrt = prrt;
    }
  if (__glibc_likely (mode == FE_TONEAREST))
    {
      rm = (ir << EXPONENT_WIDTH | UINT64_C (1) << 63) >> EXPONENT_WIDTH;
      rt = u128_mul (u128_from_u64 (rm), u128_from_u64 (am));
      rrt = u128_add (rrt, u128_from_u64 (am >> 2));
      rrt = u128_add (rrt, rt);
      ir += u128_high (rrt) >> 63;
    }
  else
    ir += mode == FE_UPWARD;
  return asdouble (ir);
}

#ifndef SECTION
# define SECTION
#endif

SECTION
double
__rsqrt (double x)
{
  uint64_t ix = asuint64 (x);
  double r;
  int e = ix >> MANTISSA_WIDTH;
  if (__glibc_unlikely (e == 0))
    {
      /* 0 <= x < 0x1p-1022.  */
      if (__glibc_unlikely (ix == 0))
	return __math_divzero (0);
      r = sqrt (x) / x;
    }
  else if (__glibc_likely (e < 0x7fd))
    /* Common case: 0x1p-1022 <= x < 0x1p1022.  */
    r = (1.0 / x) * sqrt (x);
  else if (__glibc_likely (e < 0x7ff))
    /* 0x1p1022 <= x < 0x1p1024: avoid a spurious underflow in 1/x.  */
    r = (4.0 / x) * (0.25 * sqrt (x));
  else
    {
      /* NaN, Inf, x <= 0.  */
      uint64_t ax = ix << 1;
      if (ax == 0)
	return __math_divzero (1); /* x = -0.  */
      if (ax > UINT64_C (0x7ff) << 53)
	return x + x; /* NaN.  */
      if (e >> 11)
	return __math_invalid (x); /* x < 0.  */
      return 0.0; /* +Inf.  */
    }
  /* Use the Newton iteration r' = r - r/2*(r^2*x-1).  */
  double rx = r * x, drx = fma (r, x, -rx);
  /* rx + drx approximates r*x with about 106 bits.  */
  double h = fma (r, rx, -1.0) + r * drx, dr = (r * 0.5) * h;
  /* h approximates r^2*x-1 and dr the correction term r/2*(r^2*x-1).  */
  double rf = r - dr;
  uint64_t idr = asuint64 (dr), ir = asuint64 (r);
  int de = (ir >> MANTISSA_WIDTH) - ((idr >> MANTISSA_WIDTH) & 0x7ff);
  uint64_t d = (idr + 18) & (~UINT64_C (0) >> 13);
  if (__glibc_unlikely (d <= 23 || de > 100))
    rf = as_rsqrt_refine (rf, x);
  return rf;
}
#ifndef __rsqrt
libm_alias_double (__rsqrt, rsqrt)
#endif
