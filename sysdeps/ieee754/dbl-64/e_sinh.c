/* Correctly rounded hyperbolic sine for binary64 values.

Copyright (c) 2023-2026 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/sinh/sinh.c, revision 8c2c3473).

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

#include <array_length.h>
#include <math.h>
#include <stdint.h>
#include <libm-alias-finite.h>
#include <libm-alias-double.h>
#include <math-svid-compat.h>
#include <ddcoremath.h>
#include "e_coshsinh_common.h"
#include "e_sinh_data.h"
#include "e_coshsinh_data.h"
#include "math_config.h"

#ifndef SECTION
# define SECTION
#endif

static double __attribute__ ((noinline))
as_sinh_zero (double x)
{
  double x2 = x * x, x2l = fma (x, x, -x2);
  double y2 = x2
	      * (0x1.6124613aef206p-33
		 + x2 * (0x1.ae7f36beea815p-41 + x2 * 0x1.95785063cd974p-49));
  double y1 = polydd_coshsinh (x2, x2l, 5, CH, &y2);
  y1 = mulddd3 (y1, y2, x, &y2);
  y1 = muldd2 (y1, y2, x2, x2l, &y2);
  double y0 = fasttwosum (x, y1, &y1);
  y1 = fasttwosum (y1, y2, &y2);
  uint64_t t = asuint64 (y1);
  if (__glibc_unlikely (!(t & (~0ul >> 12))))
    {
      uint64_t w = asuint64 (y2);
      if ((w ^ t) >> 63)
	t--;
      else
	t++;
      y1 = asdouble (t);
    }
  return y0 + y1;
}

static __attribute__ ((noinline)) double
as_sinh_database (double x, double f)
{
  int a = 0, b = array_length (DB) - 1, m = (a + b) / 2;
  double ax = fabs (x);
  while (a <= b)
    {
      if (DB[m][0] < ax)
	a = m + 1;
      else if (DB[m][0] == ax)
	{
	  f = copysign (1, x) * DB[m][1]
	      + copysign (1, x) * DB[m][2];
	  break;
	}
      else
	b = m - 1;
      m = (a + b) / 2;
    }
  return f;
}

SECTION
double
__sinh (double x)
{
  const double s = 0x1.71547652b82fep+12;
  double ax = fabs (x), v0 = fma (ax, s, 0x1.8000002p+26);
  uint64_t jt = asuint64 (v0);
  uint64_t v = jt;
  uint64_t tt = ~((1 << 26) - 1l);
  v &= tt;
  double t = asdouble (v) - 0x1.8p26;
  uint64_t aix = asuint64 (ax);
  if (__glibc_unlikely (aix < 0x3fd0000000000000ull))
    { // |x| < 0x1p-2
      if (__glibc_unlikely (aix < 0x3e57137449123ef7ull))
	// |x| < 0x1.7137449123ef7p-26
	return fma (x, 0x1p-55, x);
      static const double c[] = { 0x1.5555555555555p-3, 0x1.1111111111087p-7,
				  0x1.a01a01a12e1c3p-13, 0x1.71de2e415aa36p-19,
				  0x1.aed2bff4269e6p-26 };
      double x2 = x * x, x3 = x2 * x, x4 = x2 * x2,
	     p
	     = x3
	       * ((c[0] + x2 * c[1]) + x4 * ((c[2] + x2 * c[3]) + x4 * c[4]));
      double e = x3 * 0x1.9p-53, lb = x + (p - e), ub = x + (p + e);
      if (lb == ub)
	return lb;
      return as_sinh_zero (x);
    }
  if (__glibc_unlikely (aix > 0x408633ce8fb9f87dull))
    { // |x| >~ 710.47586
      if (aix >= 0x7ff0000000000000ull)
	return x + x; // nan Inf
      return __math_oflow_value (copysign (0x1p1023, x) * 2.0);
    }
  int64_t il = ((uint64_t) jt << 14) >> 40, jl = -il;
  int64_t i1 = il & 0x3f, i0 = (il >> 6) & 0x3f, ie = il >> 12;
  int64_t j1 = jl & 0x3f, j0 = (jl >> 6) & 0x3f, je = jl >> 12;
  double sp = asdouble ((uint64_t) (1022 + ie) << 52),
	 sm = asdouble ((uint64_t) (1022 + je) << 52);
  double t0h = T0[i0][1], t0l = T0[i0][0];
  double t1h = T1[i1][1], t1l = T1[i1][0];
  double th = t0h * t1h,
	 tl = t0h * t1l + t1h * t0l + fma (t0h, t1h, -th);
  const double l2h = 0x1.62e42ffp-13, l2l = 0x1.718432a1b0e26p-47;
  double dx = (ax - l2h * t) + l2l * t, dx2 = dx * dx, mx = -dx;
  static const double ch[]
      = { 0x1p+0, 0x1p-1, 0x1.5555555aaaaaep-3, 0x1.55555551c98cp-5 };
  double pp = dx * ((ch[0] + dx * ch[1]) + dx2 * (ch[2] + dx * ch[3]));
  double rh, rl;
  if (__glibc_unlikely (aix > 0x4014000000000000ull))
    { // |x| > 5
      if (__glibc_unlikely (aix > 0x40425e4f7b2737faull))
	{ // |x| >~ 36.736801
	  sp = asdouble ((1021 + ie) << 52);
	  rh = th;
	  rl = tl + th * pp;
	  rh *= copysign (1, x);
	  rl *= copysign (1, x);
	  double e = 0.11e-18 * th, lb = rh + (rl - e), ub = rh + (rl + e);
	  if (lb == ub)
	    return (lb * sp) * 2;

	  th = as_exp_accurate (ax, t, th, tl, &tl);
	  th = fasttwosum (th, tl, &tl);
	  th *= copysign (1, x);
	  tl *= copysign (1, x);
	  uint64_t uh = asuint64 (th), ul = asuint64 (tl);
	  int64_t eh = (uh >> 52) & 0x7ff, el = (ul >> 52) & 0x7ff,
		  ml = (ul + 8) & (~0ul >> 12);
	  th += tl;
	  th *= 2;
	  th *= asuint64 (sp);
	  if (ml <= 16 || eh - el > 103)
	    return as_sinh_database (x, th);
	  return th;
	}
      double q0h = T0[j0][1], q1h = T1[j1][1], qh = q0h * q1h;
      th *= sp;
      tl *= sp;
      qh *= sm;
      double pm = mx * ((ch[0] + mx * ch[1]) + dx2 * (ch[2] + mx * ch[3]));
      double em = qh + qh * pm;
      rh = th;
      rl = (tl - em) + th * pp;

      rh *= copysign (1, x);
      rl *= copysign (1, x);
      // fails with e = 0.1162e-18*rh and x=0x1.4059050000564p+2 (rndz, no fma)
      double e = 0.117e-18 * rh, lb = rh + (rl - e), ub = rh + (rl + e);
      if (lb == ub)
	return lb;

      th = as_exp_accurate (ax, t, th, tl, &tl);
      if (__glibc_unlikely (aix > 0x403f666666666666ull))
	{
	  rh = th - qh;
	  rl = ((th - rh) - qh) + tl;
	}
      else
	{
	  qh = q0h * q1h;
	  double q0l = T0[j0][0], q1l = T1[j1][0];
	  double ql = q0h * q1l + q1h * q0l + fma (q0h, q1h, -qh);
	  qh *= sm;
	  ql *= sm;
	  qh = as_exp_accurate (-ax, -t, qh, ql, &ql);
	  rh = th - qh;
	  rl = (((th - rh) - qh) - ql) + tl;
	}
    }
  else
    {
      double q0h = T0[j0][1], q0l = T0[j0][0];
      double q1h = T1[j1][1], q1l = T1[j1][0];
      double qh = q0h * q1h,
	     ql = q0h * q1l + q1h * q0l + fma (q0h, q1h, -qh);
      th *= sp;
      tl *= sp;
      qh *= sm;
      ql *= sm;
      double pm = mx * ((ch[0] + mx * ch[1]) + dx2 * (ch[2] + mx * ch[3]));
      double fph = th, fpl = tl + th * pp;
      double fmh = qh, fml = ql + qh * pm;

      rh = fph - fmh;
      rl = ((fph - rh) - fmh) - fml + fpl;
      rh *= copysign (1, x);
      rl *= copysign (1, x);
      double e = 0.33e-18 * rh, lb = rh + (rl - e), ub = rh + (rl + e);
      if (lb == ub)
	return lb;
      th = as_exp_accurate (ax, t, th, tl, &tl);
      qh = as_exp_accurate (-ax, -t, qh, ql, &ql);
      rh = th - qh;
      rl = ((th - rh) - qh) - ql + tl;
    }
  rh = fasttwosum (rh, rl, &rl);
  uint64_t uh = asuint64 (rh), ul = asuint64 (rl);
  int64_t eh = (uh >> 52) & 0x7ff, el = (ul >> 52) & 0x7ff,
	  ml = (ul + 8) & (~0ul >> 12);
  rh *= copysign (1, x);
  rl *= copysign (1, x);
  rh += rl;
  if (__glibc_unlikely (ml <= 16 || eh - el > 103))
    return as_sinh_database (x, rh);
  return rh;
}

#ifndef __sinh
strong_alias (__sinh, __ieee754_sinh)
# if LIBM_SVID_COMPAT
versioned_symbol (libm, __sinh, sinh, GLIBC_2_44);
libm_alias_double_other (__sinh, sinh)
# else
libm_alias_double (__sinh, sinh)
# endif
libm_alias_finite (__ieee754_sinh, __sinh)
#endif
