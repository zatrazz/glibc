/* Correctly-rounded sincos function for binary64 value.

Copyright (c) 2026 Alexei Sibidanov <sibid@uvic.ca> and Paul Zimmermann.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/sincos/sincos.c, revision 05075c2c).

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

/* Fast path for 2^-27 <= |x| < 2^31, where sbit is the sign bit of x (0
   for x > 0, 1 for x < 0).  See the proof of correctness in sin.pdf.  */
static inline void
sincos_moderate (double x, int sbit, double *sinx, double *cosx)
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
  int cbit = (j >> 14) & 1;
  sbit ^= cbit;
  int i1 = (j >> 7) & 0x7f, i2 = j & 0x7f;

  double s1h = U1[i1][0], s1l = U1[i1][1];
  double c1h = U1[i1][2], c1l = U1[i1][3];
  double s2h = U2[i2][0], s2l = U2[i2][1];
  double c2h = U2[i2][2], c2l = U2[i2][3];

  /* s12h + s12l approximates sin(t1+t2) = sin(t1)*cos(t2)+cos(t1)*sin(t2)
     and c12h + c12l approximates cos(t1+t2) = cos(t1)*cos(t2)-sin(t1)*sin(t2),
     where t1=i1*pi/2^7 and t2=i2*pi/2^14.  */
  double s12l, s12h = muldd (s1h, s1l, c2h, c2l, &s12l), tl, th;
  th = muldd (c1h, c1l, s2h, s2l, &tl);
  s12h = fastsum (s12h, s12l, th, tl, &s12l);

  double c12l, c12h = muldd (c1h, c1l, c2h, c2l, &c12l);
  th = muldd (s1h, s1l, s2h, s2l, &tl);
  c12h = fastsum (c12h, c12l, -th, -tl, &c12l);

  /* For |r| <= 2^-13.339, the polynomial r - 0x1.55555553068fp-3 * r^3
     approximates sin(r) with absolute error < 2^-76.494, and the polynomial
     0.5 * r^2 - 0x1.55555553bfd3p-5 * r^4 approximates 1-cos(r) with
     absolute error < 2^-92.723 (cf sinmoderate.sollya).  */
  double sh = r * (1.0 - 0x1.55555553068fp-3 * r2);
  double ch = r2 * (0.5 - 0x1.55555553bfd3p-5 * r2);

  s12l -= s12h * ch - c12h * sh;
  c12l -= c12h * ch + s12h * sh;
  static const double sgn[] = { 1.0, -1.0 };
  s12h *= sgn[sbit];
  s12l *= sgn[sbit];
  c12h *= sgn[cbit];
  c12l *= sgn[cbit];

  static const double eps = 0x1.dep-64;
  double lb = s12h + (s12l - eps), ub = s12h + (s12l + eps);
  if (__glibc_likely (ub == lb))
    *sinx = lb;
  else if (ax < 0x1p-16)
    *sinx = sin_small_accurate (x);
  else
    *sinx = __sin_large_accurate (x);
  lb = c12h + (c12l - eps);
  ub = c12h + (c12l + eps);
  if (__glibc_likely (ub == lb))
    *cosx = lb;
  else
    *cosx = __cos_large_accurate (x);
}

/* Fast path for |x| >= 2^31.  */
static void __attribute__ ((noinline))
sincos_large (double x, double *sinx, double *cosx)
{
  double ax = fabs (x);
  double r;
  /* Now x/(2pi) ~ k + j/2^15 + r with 0 <= r < 2^-15.  */
  uint64_t j = reduce_large (&r, ax);
  int sbit = (x > 0) ? 0 : 1;

  double r2 = r * r;
  /* Reduction by an odd multiple of pi?  */
  int cbit = j >> 14;
  sbit ^= cbit;
  int i1 = (j >> 7) & 0x7f, i2 = j & 0x7f;

  double s1h = U1[i1][0], s1l = U1[i1][1];
  double c1h = U1[i1][2], c1l = U1[i1][3];
  double s2h = U2[i2][0], s2l = U2[i2][1];
  double c2h = U2[i2][2], c2l = U2[i2][3];

  double s12l, s12h = muldd (s1h, s1l, c2h, c2l, &s12l), tl, th;
  th = muldd (c1h, c1l, s2h, s2l, &tl);
  s12h = fastsum (s12h, s12l, th, tl, &s12l);

  double c12l, c12h = muldd (c1h, c1l, c2h, c2l, &c12l);
  th = muldd (s1h, s1l, s2h, s2l, &tl);
  c12h = fastsum (c12h, c12l, -th, -tl, &c12l);

  double sh = r * (0x1.921fb54442d18p2 - 0x1.4abbcdb6b26d1p5 * r2);
  double ch = r2 * (0x1.3bd3cc9be45dep4 - 0x1.03c1eee483083p6 * r2);

  s12l -= s12h * ch - c12h * sh;
  c12l -= c12h * ch + s12h * sh;
  static const double sgn[] = { 1.0, -1.0 };
  s12h *= sgn[sbit];
  s12l *= sgn[sbit];
  c12h *= sgn[cbit];
  c12l *= sgn[cbit];
  /* Fails with eps=0x1.7fp-64 and x=0x1.54a9ad28f0a25p+225 (rounding toward
     zero, no FMA).  */
  static const double eps = 0x1.01p-63;
  double lb = s12h + (s12l - eps), ub = s12h + (s12l + eps);
  if (__glibc_likely (ub == lb))
    *sinx = lb;
  else
    *sinx = __sin_large_accurate (x);
  lb = c12h + (c12l - eps);
  ub = c12h + (c12l + eps);
  if (__glibc_likely (ub == lb))
    *cosx = lb;
  else
    *cosx = __cos_large_accurate (x);
}

SECTION
void
__sincos (double x, double *sinx, double *cosx)
{
  uint64_t t = asuint64 (x);
  int e = (t >> MANTISSA_WIDTH) & 0x7ff;
  /* For |x| < 2^-27, sin(x) rounds to x and cos(x) rounds to 1 (to
     nearest).  */
  if (__glibc_unlikely (e < EXPONENT_BIAS - 27))
    {
      *cosx = fma (x, -x, 1.0);
      if ((t << 1) == 0)
	{
	  *sinx = x;
	  return;
	}
      /* The Taylor expansion of sin(x) is x - x^3/6 around zero.  There is
	 an underflow when 0 < |x| < 2^-1022 or when |x| = 2^-1022 and
	 rounding towards zero.  */
      *sinx = __math_check_uflow_lt (fma (x, -0x1p-54, x), 0x1p-1022);
      return;
    }
  /* |x| < 2^31.  */
  if (__glibc_likely (e < EXPONENT_BIAS + 31))
    {
      sincos_moderate (x, t >> 63, sinx, cosx);
      return;
    }
  /* NaN, +Inf and -Inf.  */
  if (__glibc_unlikely (e == 0x7ff))
    {
      *sinx = *cosx = __math_invalid (x);
      return;
    }
  /* Now |x| >= 2^31.  */
  sincos_large (x, sinx, cosx);
}
#ifndef __sincos
libm_alias_double (__sincos, sincos)
#endif
