/* Correctly-rounded cosine function for binary64 value.

Copyright (c) 2026 Paul Zimmermann.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/cos/cos.c, revision 9b821fc6), which was
adapted from src/binary64/sin/sin.c.

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

/* Fast path for 2^-27 <= x < 2^31 (adapted from sin_moderate).  See the
   proof of correctness in cos.pdf.  */
static inline double
cos_moderate (double x)
{
  /* |invpi/2^14 - 1/pi| < 2^-55.496.  */
  static const double invpi = 0x1.45f306dc9c883p+12;
  double k = roundeven_finite (invpi * x);
  /* |2^14*(pih + pil) + pi| < 2^-108.041, and rh is exact.  */
  double rh = fma (k, pih, x), rl = k * pil;

  /* |r| < 2^-13.339 (see sin.pdf).  */
  double r = rh + rl;
  double r2 = r * r;
  int64_t j = k;
  /* Reduction by an odd multiple of pi?  */
  int sbit = (j >> 14) & 1;
  int i1 = (j >> 7) & 0x7f, i2 = j & 0x7f;
  double c1h, c1l, c2h, c2l;
  /* c1h approximates cos(t1)*cos(t2) where t1=i1*pi/2^7 and t2=i2*pi/2^14,
     and c2h approximates sin(t1)*sin(t2).  */
  c1h = muldd (U1[i1][2], U1[i1][3], U2[i2][2], U2[i2][3], &c1l);
  c2h = muldd (U2[i2][0], U2[i2][1], U1[i1][0], U1[i1][1], &c2l);
  /* Ch+Cl approximates cos(t1+t2) = cos(t1)*cos(t2)-sin(t1)*sin(t2).  */
  double Ch, Cl;
  Ch = fastsum (c1h, c1l, -c2h, -c2l, &Cl);
  /* Sh approximates sin(t1+t2) = sin(t1)*cos(t2)+cos(t1)*sin(t2).  */
  double Sh = U1[i1][0] * U2[i2][2] + U1[i1][2] * U2[i2][0];

  /* For |r| <= 2^-13.339, the polynomial r - 0x1.55555553068fp-3 * r^3
     approximates sin(r) with absolute error < 2^-76.494, and the polynomial
     -0.5 * r^2 + 0x1.55555553bfd3p-5 * r^4 approximates cos(r)-1 with
     absolute error < 2^-92.723 (cf sinmoderate.sollya).  */
  double sh = r * (1.0 - 0x1.55555553068fp-3 * r2);
  double ch = r2 * (-0.5 + 0x1.55555553bfd3p-5 * r2);
  double fh = Ch, fl = Cl + Ch * ch - Sh * sh;
  static const double sgn[] = { 1.0, -1.0 };
  fh = sgn[sbit] * fh;
  fl = sgn[sbit] * fl;
  static const double eps = 0x1.dep-64;
  double lb = fh + (fl - eps), ub = fh + (fl + eps);
  if (__glibc_likely (ub == lb))
    return lb;
  return __cos_large_accurate (x);
}

/* Fast path for x >= 2^31.  */
static double __attribute__ ((noinline))
cos_large (double x)
{
  double r;
  /* Now x/(2pi) ~ k + j/2^15 + r with 0 <= r < 2^-15.  */
  uint64_t j = reduce_large (&r, x);

  double r2 = r * r;
  /* Reduction by an odd multiple of pi?  */
  int sbit = j >> 14;
  int i1 = (j >> 7) & 0x7f, i2 = j & 0x7f;
  double c1h, c1l, c2h, c2l;
  c1h = muldd (U1[i1][2], U1[i1][3], U2[i2][2], U2[i2][3], &c1l);
  c2h = muldd (U2[i2][0], U2[i2][1], U1[i1][0], U1[i1][1], &c2l);
  double Ch, Cl;
  Ch = fastsum (c1h, c1l, -c2h, -c2l, &Cl);
  double Sh = U1[i1][0] * U2[i2][2] + U1[i1][2] * U2[i2][0];

  double sh = r * (0x1.921fb54442d18p2 - 0x1.4abbcdb6b26d1p5 * r2);
  double ch = r2 * (-0x1.3bd3cc9be45dep4 + 0x1.03c1eee483083p6 * r2);
  double fh = Ch, fl = Cl + Ch * ch - Sh * sh;
  static const double sgn[] = { 1.0, -1.0 };
  fh = sgn[sbit] * fh;
  fl = sgn[sbit] * fl;
  /* Fails with eps=0x1.7fp-64 and x=0x1.64dd7934d0fdfp+953 (rounding toward
     zero, no FMA).  */
  static const double eps = 0x1.01p-63;
  double lb = fh + (fl - eps), ub = fh + (fl + eps);
  if (__glibc_likely (lb == ub))
    return lb;
  return __cos_large_accurate (x);
}

SECTION
double
__cos (double x)
{
  double z = fabs (x);
  uint64_t t = asuint64 (z);
  int e = t >> MANTISSA_WIDTH;
  /* Deal with tiny x to avoid underflow.  For |x| < 2^-27, cos(x) rounds to
     1 to nearest.  */
  if (__glibc_unlikely (e < EXPONENT_BIAS - 27))
    return fma (x, -x, 1.0);
  /* |x| < 2^31.  */
  if (__glibc_likely (e < EXPONENT_BIAS + 31))
    return cos_moderate (z);
  /* NaN, +Inf and -Inf.  */
  if (__glibc_unlikely (e == 0x7ff))
    return __math_invalid (x);
  /* Now |x| >= 2^31.  */
  return cos_large (z);
}
#ifndef __cos
libm_alias_double (__cos, cos)
#endif
