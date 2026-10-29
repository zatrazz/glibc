/* Correctly-rounded sine function for binary64 value.

Copyright (c) 2026 Paul Zimmermann and Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/sin/sin.c, revision a3fe413f).

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

/* References:
   [1] Handbook of Floating-Point Arithmetic (2nd edition),
   Muller, Jean-Michel and Brunie, Nicolas and de Dinechin, Florent and
   Jeannerod, Claude-Pierre and Joldes, Mioara and Lefèvre, Vincent and
   Melquiond, Guillaume and Revol, Nathalie and Torres, Serge,
   Birkhäuser, 2018.
   [2] Computing hard-to-round cases of sin, cos, tan in double precision,
   Vincent Lefèvre, Tue Ly, Paul Zimmermann,
   ARITH 2026 - 33rd IEEE International Symposium on Computer Arithmetic,
   2026.  */

#include <libm-alias-double.h>
#include <math.h>
#include <math_uint128.h>
#include <stdint.h>
#include <ddcoremath.h>
#include "math_config.h"
#include "s_sin_common.h"

#ifndef SECTION
# define SECTION
#endif

/* Accurate path for |x| >= 2^-16.  */
static double __attribute__ ((noinline, cold))
sin_large_accurate (double x)
{
  u128 r;
  int neg;
  uint64_t k = reduce_large_acc (&r, &neg, x);

  /* Replace r by 8*r.  */
  r = u128_lshift (r, 3);
  u128 u2 = mhUU (r, r), u4 = mhUU (u2, u2);
  u128 u2h = u128_rshift (u2, 64);

  /* x/(2*pi) mod 1 = k/2^13 + r + eps with |r| <= 2^-14 and
     0 <= eps < 2^-127.999 then
       sin(x) ~ sin(pi*k/2^12 + 2*pi*r)
	      ~ sin(pi*k/2^12)*cos(2*pi*r) + cos(pi*k/2^12)*sin(2*pi*r)
     Write k = 2^12*s + 2^6*i1 + i2 and t1=pi*i1/2^6, t2 = pi*i2/2^12
     then sin(pi*k/2^12) = (-1)^s*[sin(t1)*cos(t2)+cos(t1)*sin(t2)]
     and  cos(pi*k/2^12) = (-1)^s*[cos(t1)*cos(t2)-sin(t1)*sin(t2)].  */
  int sbit = (x > 0) ? 0 : 1;
  sbit = sbit ^ (k >> 12);
  int i1 = (k >> 6) & 0x3f, i2 = k & 0x3f;

  /* Approximate |sin(z)| in s1u/2^128, using sin(pi-x) = sin(x).  */
  u128 s1 = u128_load ((i1 <= 32) ? S1u[i1] : S1u[64 - i1]);
  /* Since cos(x) = sin(x+pi/2), |C1u[i1]| = |S1u[(i1+32) mod 64]|.  */
  u128 c1 = u128_load ((i1 <= 32) ? S1u[32 - i1] : S1u[i1 - 32]);
  u128 s1u = mhUU (s1, u128_load (C2u[i2]));
  u128 t = mhUU (c1, u128_load (S2u[i2]));
  /* s1u/2^128 approximates sin(t1)*cos(t2), and t/2^128 approximates (up
     to sign) sin(t2)*cos(t1), thus s1u +/- t approximates sin(z=t1+t2), but
     since sin(z) >= 0 (0 <= z < pi), s1u +/- t >= 0.  */
  s1u = (i1 < 32) ? u128_add (s1u, t) : u128_sub (s1u, t);

  /* Approximate cos(z) in c1u/2^128.  */
  u128 c1u = mhUU (c1, u128_load (C2u[i2]));
  t = mhUU (s1, u128_load (S2u[i2]));
  c1u = (i1 < 32) ? u128_sub (c1u, t) : u128_add (c1u, t);

  /* Sr/2^128 approximates |sin(2*pi*r)| and Cr/2^128 approximates
     cos(2*pi*r).  */
  u128 Sr = evalPS (r, u2, u2h, u4);
  u128 Cr = evalPC (u2, u2h, u4);

  /* Now combine: sin(x) ~ s1*C + c1*S.  */
  s1u = mhUU (s1u, Cr);
  c1u = mhUU (c1u, Sr);

  /* s1u/2^128 approximates sin(z)*cos(r) which is always >= 0, while
     c1u/2^128 approximates cos(z)*sin(r), where cos(z) > 0 for i1 < 32,
     and cos(z) <= 0 for i1 >= 32, and sign(r) has the sign of r.  */
  if ((i1 < 32) ^ neg)
    /* cos(z) * r > 0.  */
    s1u = u128_add (s1u, c1u);
  else if (u128_lt (s1u, c1u))
    {
      s1u = u128_sub (c1u, s1u);
      sbit ^= 1;
    }
  else
    s1u = u128_sub (s1u, c1u);
  return u128_tod (s1u, sbit);
}

/* Fast path for 2^-26 < |x| < 2^31, where sbit is the sign bit of x (0 for
   x > 0, 1 for x < 0).  See the proof of correctness in sin.pdf.  */
static inline double
sin_moderate (double x, int sbit)
{
  double ax = fabs (x);
  /* |invpi/2^14 - 1/pi| < 2^-55.496.  */
  static const double invpi = 0x1.45f306dc9c883p+12;
  double k = roundeven_finite (invpi * ax);
  /* |2^14*(pih + pil) + pi| < 2^-108.041, and rh is exact.  */
  double rh = fma (k, pih, ax), rl = k * pil;

  /* |r| < 2^-13.339 (see sin.pdf).  */
  double r = rh + rl;
  double r2 = r * r;
  int64_t j = k;
  /* Reduction by an odd multiple of pi?  */
  sbit ^= (j >> 14) & 1;
  int i1 = (j >> 7) & 0x7f, i2 = j & 0x7f;
  double s1h, s1l, s2h, s2l;
  s1h = muldd (U1[i1][0], U1[i1][1], U2[i2][2], U2[i2][3], &s1l);
  s2h = muldd (U2[i2][0], U2[i2][1], U1[i1][2], U1[i1][3], &s2l);
  double Sh, Sl;
  Sh = fastsum (s1h, s1l, s2h, s2l, &Sl);
  double Ch = U1[i1][2] * U2[i2][2] - U1[i1][0] * U2[i2][0];

  /* For |r| <= 2^-13.339, the polynomial r - 0x1.55555553068fp-3 * r^3
     approximates sin(r) with absolute error < 2^-76.494, and the polynomial
     -0.5 * r^2 + 0x1.55555553bfd3p-5 * r^4 approximates cos(r)-1 with
     absolute error < 2^-92.723 (cf sinmoderate.sollya).  */
  double sh = r * (1.0 - 0x1.55555553068fp-3 * r2);
  double ch = r2 * (-0.5 + 0x1.55555553bfd3p-5 * r2);
  double fh = Sh, fl = Sl + Sh * ch + Ch * sh;
  static const double sgn[] = { 1.0, -1.0 };
  fh = sgn[sbit] * fh;
  fl = sgn[sbit] * fl;
  static const double eps = 0x1.dep-64;
  double lb = fh + (fl - eps), ub = fh + (fl + eps);
  if (__glibc_likely (ub == lb))
    return lb;
  if (ax < 0x1p-16)
    return sin_small_accurate (x);
  return sin_large_accurate (x);
}

/* Fast path for |x| >= 2^31.  */
static double __attribute__ ((noinline))
sin_large (double x)
{
  double ax = fabs (x);
  double r;
  /* Now x/(2pi) ~ k + j/2^15 + r with 0 <= r < 2^-15.  */
  uint64_t j = reduce_large (&r, ax);
  int sbit = (x > 0) ? 0 : 1;

  double r2 = r * r;
  /* Reduction by an odd multiple of pi?  */
  sbit = sbit ^ (j >> 14);
  int i1 = (j >> 7) & 0x7f, i2 = j & 0x7f;
  double s1h, s1l, s2h, s2l;
  s1h = muldd (U1[i1][0], U1[i1][1], U2[i2][2], U2[i2][3], &s1l);
  s2h = muldd (U2[i2][0], U2[i2][1], U1[i1][2], U1[i1][3], &s2l);
  double Sh, Sl;
  Sh = fastsum (s1h, s1l, s2h, s2l, &Sl);
  double Ch = U1[i1][2] * U2[i2][2] - U1[i1][0] * U2[i2][0];

  double sh = r * (0x1.921fb54442d18p2 - 0x1.4abbcdb6b26d1p5 * r2);
  double ch = r2 * (-0x1.3bd3cc9be45dep4 + 0x1.03c1eee483083p6 * r2);
  double fh = Sh, fl = Sl + Sh * ch + Ch * sh;
  static const double sgn[] = { 1.0, -1.0 };
  fh = sgn[sbit] * fh;
  fl = sgn[sbit] * fl;
  /* Fails with eps=0x1.7fp-64 and x=0x1.54a9ad28f0a25p+225 (rounding toward
     zero, no FMA).  */
  static const double eps = 0x1.01p-63;
  double lb = fh + (fl - eps), ub = fh + (fl + eps);
  if (__glibc_likely (lb == ub))
    return lb;
  return sin_large_accurate (x);
}

SECTION
double
__sin (double x)
{
  uint64_t t = asuint64 (x);
  int e = (t >> MANTISSA_WIDTH) & 0x7ff;
  /* Deal with tiny x to avoid underflow.  */
  if (__glibc_unlikely (e < EXPONENT_BIAS - 26))
    {
      /* For |x| < 2^-26, |sin(x) - x| < 1/2 ulp.  */
      if ((t << 1) == 0)
	return x;
      /* The Taylor expansion of sin(x) is x - x^3/6 around zero.  There is
	 an underflow when 0 < |x| < 2^-1022 or when |x| = 2^-1022 and
	 rounding towards zero.  */
      return __math_check_uflow_lt (fma (x, -0x1p-54, x), 0x1p-1022);
    }
  /* |x| < 2^31.  */
  if (__glibc_likely (e < EXPONENT_BIAS + 31))
    return sin_moderate (x, t >> 63);
  /* NaN, +Inf and -Inf.  */
  if (__glibc_unlikely (e == 0x7ff))
    return __math_invalid (x);
  /* Now |x| >= 2^31.  */
  return sin_large (x);
}
#ifndef __sin
libm_alias_double (__sin, sin)
#endif
