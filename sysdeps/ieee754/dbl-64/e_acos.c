/* Correctly-rounded arc cosine of binary64 value.

Copyright (c) 2024-2025 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/acos/acos.c, revision b748a2d2).

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

#include <math.h>
#include <stdint.h>
#include <libm-alias-finite.h>
#include "math_config.h"
#include "e_acos_data.h"

static inline double
fasttwosum (double x, double y, double *e)
{
  double s = x + y, z = s - x;
  *e = y - z;
  return s;
}

/* Reference: Handbook of Floating-Point Arithmetic, Algorithm 4.4.
   Theorem 4.1 from "On the Robustness of the 2Sum and Fast2Sum Algorithms"
   by Sylvie Boldo, Stef Graillat and Jean-Michel Muller,
   ACM Transactions on Mathematical Software, 2017 says:
   t = (a+b) - s + alpha with |alpha| <= 2^(-p+1) ulp(s) [here p=53] */
static inline double
twosum (double a, double b, double *t)
{
  double s = a + b;
  double a_prime = s - b;
  double b_prime = s - a_prime;
  double delta_a = a - a_prime;
  double delta_b = b - b_prime;
  *t = delta_a + delta_b;
  return s;
}

static inline double
fastsum (double xh, double xl, double yh, double yl, double *e)
{
  double sl, sh = fasttwosum (xh, yh, &sl);
  *e = (xl + yl) + sl;
  return sh;
}

static inline double
sum (double xh, double xl, double ch, double cl, double *l)
{
  double sl, sh = twosum (xh, ch, &sl);
  *l = (xl + cl) + sl;
  return sh;
}

static inline double
muldd (double xh, double xl, double ch, double cl, double *l)
{
  double ahhh = xh * ch;
  *l = (xh * cl + xl * ch) + fma (xh, ch, -ahhh);
  return ahhh;
}

static inline double
polydd (double xh, double xl, int n, const double c[][2], double *l)
{
  int i = n - 1;
  double ch = fasttwosum (c[i][0], *l, l), cl = c[i][1] + *l;
  while (--i >= 0)
    {
      ch = muldd (xh, xl, ch, cl, &cl);
      ch = fastsum (c[i][0], c[i][1], ch, cl, &cl);
    }
  *l = cl;
  return ch;
}

static double __attribute__ ((noinline, cold)) as_acos_refine (double, double);

double
__ieee754_acos (double x)
{
  // coefficients of a polynomial approximation of asin(x):
  // asin(x) = x*(cc[j][0] + cc[j][1] + t*P(t, cc[j] + 2))
  // where t = x^2 - j/128

  uint64_t ix = asuint64 (x);
  uint64_t ax = ix << 1;
  double t, z, zl, jd, f0h, f0l, eps;
  if (ax > UINT64_C (0x7fc0000000000000))
    { // |x|>0.5
      int64_t k = ix >> 63;
      f0h = off[k][0];
      f0l = off[k][1];
      if (__glibc_unlikely (ax >= UINT64_C (0x7fe0000000000000)))
	{ // |x| >= 1
	  if (ax == UINT64_C (0x7fe0000000000000))
	    return f0h + f0l; // |x| = 1
	  if (ax > UINT64_C (0xffe0000000000000))
	    return x + x;	     // nan
	  return __math_invalid (x); // |x|>1
	}
      // for x>0.5 we use range reduction for double angle formula
      // acos(x) = 2*asin((1-x)/2) and for x<-0.5 acos(x) = pi -
      // 2*asin((1-x)/2)
      t = 2 - 2 * fabs (x);
      jd = roundeven_finite (t * 0x1p5);
      z = copysign (sqrt (t), x);
      zl = fma (z, z, -t) * ((-0.5 / t) * z);
      t = 0.25 * t - jd * 0x1p-7;
      // fails with 0x1.8bp-52 for x=-0x1.3e827a2cd6d51p-1 (no FMA)
      eps = fabs (z * t) * 0x1.8cp-52 + 0x1p-105;
    }
  else
    { // |x|<=0.5
      f0h = 0x1.921fb54442d18p+0;
      f0l = 0x1.1a62633145c07p-54;

      if (__glibc_unlikely (ax <= UINT64_C (0x7e00000000000000)))
	{ // |x| < 2^-15
	  static const double c = -0x1.5555555555555p-3;
	  /* Avoid a spurious underflow for |x| <= x0 := 0x1.cb3b3869747f4p-55;
	     moreover for |x| <= x0 we always have lb=ub, thus the accurate
	     path is never called. */
	  double v
	      = (ax <= UINT64_C (0x791967670d2e8fe8)) ? 0 : (x * x) * (c * x);
	  double h, w;
	  h = fasttwosum (f0h, -x, &w);
	  double l = v + (w + f0l);
	  static const double eps1 = 0x1.34p-79;
	  double lb = h + (l - eps1), ub = h + (l + eps1);
	  if (__glibc_unlikely (lb != ub))
	    return as_acos_refine (x, lb);
	  return lb;
	}

      // for 2^-15 <= |x| <= 0.5 we use acos(x) = pi/2 - asin(x) so the
      // argument range for asin is the same for both branches to reuse the
      // lookup tables.
      t = x * x;
      jd = roundeven_finite (t * 0x1p7);
      t = fma (x, x, -0x1p-7 * jd);
      z = -x;
      zl = 0;
      // eps < 0 for x > 0, but the rounding test is still correct
      /* for |x| < 2^-4 (case j=0), fails with 0x1.d3p-53 and
	 x=0x1.7cb54339263fbp-12;
	 for 2^-4 <= |x| < 0.5, fails with 0x1.80p-52 and
	 x=-0x1.fda6fee396f8p-2 (no FMA, rndz) */
      eps = (z * t) * 0x1.81p-52;
    }
  /* an exhaustive search was performed in [-1,-2^-4] and [2^-4,1]
     with and without FMA contraction */

  // asin(xh+xl) = (xh + xl)*(cc[j][0] + (cc[j][1] + t*Poly(t, cc[j]+2)))
  // where t = xh^2 - j/128 and j = round(128*xh^2)
  int64_t j = jd;
  const double *c = cc[j];
  double t2 = t * t,
	 d = t
	     * ((c[2] + t * c[3])
		+ t2 * ((c[4] + t * c[5]) + t2 * (c[6] + t * c[7])));
  double fh = c[0], fl = c[1] + d;
  fh = muldd (z, zl, fh, fl, &fl);
  fh = fastsum (f0h, f0l, fh, fl, &fl);
  double lb = fh + (fl - eps), ub = fh + (fl + eps);
  if (__glibc_unlikely (lb != ub))
    return as_acos_refine (x, lb);
  return lb;
}

// phi is the fast path approximation of acos(x)
__attribute__ ((noinline, cold)) static double
as_acos_refine (double x, double phi)
{
  // Consider x as cos(phi) then sin(phi) is ch + cl = sqrt(1-x^2)
  // Using angle rotation formula bring the argument close to zero
  // where the asin Taylor expansion works well:
  // acos(x) = asin(sqrt(1-x^2)) for x > 0
  // acos(x) = pi+asin(-sqrt(1-x^2)) for x < 0
  double s2 = x * x, dx2 = fma (x, x, -s2);
  // s2+dx2 = x^2
  double c2l, c2h = fasttwosum (1.0, -s2, &c2l);
  c2l -= dx2;
  c2h = fasttwosum (c2h, c2l, &c2l);
  // c2h+c2l approximates 1-x^2

  double ch = sqrt (c2h);
  /* let eps = ch^2-c2h, then c2h + c2l = ch^2 + c2l - eps,
     thus sqrt(c2h + c2l) = sqrt(ch^2*(1+(c2l-eps)/ch^2))
     ~ ch*(1 + (c2l-eps)/ch^2/2) = ch + (c2l-eps)/ch/2 */
  double cl = (c2l - fma (ch, ch, -c2h)) * (0.5 / ch);
  // now ch+cl approximates sqrt(1-x^2)

  int jf = roundeven_finite (fabs (phi - 0x1.921fb54442d18p+0)
			     * 0x1.45f306dc9c883p+4);
  // jf = round(|phi-pi/2|*64/pi)
  // sin(pi/64*j) in the double-double format (smallest term first)

  /* let y = acos(x) and assume y = pi/2 -/+ jf*pi/64 - delta,
     with |delta| < pi/128,
     where -/+ means - for x > 0, and + for x < 0:
     delta = pi/2 -/+ jf*pi/64 - y thus
     sin(delta) = sin(pi/2 -/+ jf*pi/64 - y)
		= cos(-/+jf*pi/64 - y)
		= cos(-/+jf*pi/64)*cos(y) + sin(-/+jf*pi/64)*sin(y)
		= cos(jf*pi/64)*x -/+ sin(jf*pi/64)*sqrt(1-x^2)
  */

  // 0 <= jf <= 32
  double Ch = SINCOS[32 - jf][1], Cl = SINCOS[32 - jf][0], Sh = SINCOS[jf][1],
	 Sl = SINCOS[jf][0];
  /* Ch+Cl approximates cos(jf*pi/64), Sh+Sl approximates sin(jf*pi/64)
     thus sin(delta) = (Ch+Cl)*x -/+ (Sh+Sl)*sqrt(1-x^2)
		     ~ sgn(x) * [ (Ch+Cl)*|x| - (Sh+Sl)*(ch+cl)] */

  double ax = fabs (x);
  double dsh = ax - Sh, dsl = -Sl;
  double dch = ch - Ch, dcl = cl - Cl;
  /* now |x| = Sh+Sl + dsh+dsl, ch+cl = Ch+Cl + dch+dcl
     thus sin(delta) ~ sgn(x) *
     [ (Ch+Cl)*(Sh+Sl + dsh+dsl) - (Sh+Sl)*(Ch+Cl + dch+dcl)]
     ~ sgn(x) * [(Ch+Cl)*(dsh+dsl) - (Sh+Sl)*(dch+dcl)].
     Since |delta| < pi/128 and y = pi/2 -/+ jf*pi/64 - delta,
     |dsh|, |dch| < pi/128 < 0.0246 */

#define MAGIC 0x1.8p-4
  /* Remark: we could reduce magic to 0x1.8p-5, then Cs - Sc below would
     still be exact, but this would add one exceptional case
     (x=-0x1.52f06359672cdp-2) and save one, thus there is no benefit. */
  double Sc = fma (Sh, dch, MAGIC) - MAGIC;
  double dSc = fma (Sh, dch, -Sc);
  // Sc + dSc approximates Sh*dch, with Sc multiple of 2^-56 and |Sc| < 2^-5

  double Cs = fma (Ch, dsh, MAGIC) - MAGIC;
  double dCs = fma (Ch, dsh, -Cs);
  // Cs+dCs approximates Ch*dsh, with Cs multiple of 2^-56 and |Cs| < 2^-5

  double v = Cs - Sc; // exact since |Cs - Sc| multiple of 2^-56 and < 2^-4
  // v approximates Ch*dsh - Sh*dch
  double dv = (Ch * dsl + Cl * dsh) - (Sh * dcl + Sl * dch) - (dSc - dCs);
  // v+dv approximates (Ch+Cl)*(dsh+dsl) - (Sh+Sl)*(dch+dcl)
  // thus approximates by sgn(x) * sin(delta)
  v = fasttwosum (v, dv, &dv);
  double sgn = copysign (1.0, x), jt = 32 - jf * sgn;
  // pi/2 -/+ jf*pi/64 = jt*pi/64 thus y = jt*pi/64 - delta
  // with 0 <= jt <= 64
  /* c[0]*x+c[1]*x^3+...+c[4]*x^9+ct[0]*x^11+...+ct[2]*x^15 is a
     polynomial approximation of asin(x) at x=0 */
  double dv2, v2 = muldd (v, dv, v, dv, &dv2);
  v *= -sgn;
  dv *= -sgn;
  double fl = v2 * (ct[0] + v2 * (ct[1] + v2 * ct[2])),
	 fh = polydd (v2, dv2, 5, POLYC, &fl);
  fh = muldd (v, dv, fh, fl, &fl);
  // now fh+fl approximates -delta

  /* h+l+s with h=0x1.921fb54442dp-5, l=0x1.8469898cc518p-53,
     s=-0x1.fc8f8cbb5bf6cp-102 approximates pi/64 with error bounded
     by 2^-155, thus ph+pl+ps approximates jt*pi/64 with error bounded
     by 2^-149 */
  double ph = jt * 0x1.921fb54442dp-5, pl = 0x1.8469898cc518p-53 * jt,
	 ps = -0x1.fc8f8cbb5bf6cp-102 * jt;
  // since 0 <= jt <= 64, ph and pl are exact
  pl = sum (fh, fl, pl, ps, &ps);
  ph = fasttwosum (ph, pl, &pl);
  pl = fasttwosum (pl, ps, &ps);
  ph = fasttwosum (ph, pl, &pl);
  pl = fasttwosum (pl, ps, &ps);
  uint64_t t = asuint64 (pl);
  int64_t e = ((t >> 52) & 0x7ff) - 1023;
  e = 52 - (107 + e);
  e = e < 0 ? 0 : e;
  e = e > 52 ? 52 : e;
  uint64_t m = (UINT64_C (1) << 52) - (UINT64_C (1) << e);
  e = (e == 0) ? 64 : e;
  if (__glibc_unlikely (!((t + (UINT64_C (1) << (e - 1))) & m)))
    {
      if (x == 0x1.ffffffffffdc0p-1)
	return 0x1.8000000000024p-22 + 0x1p-76;
      if (x == 0x1.53ea6c7255e88p-4)
	return 0x1.7cdacb6bbe707p+0 + 0x1p-54;
      if (x == 0x1.fd737be914578p-11)
	return 0x1.91e006d41d8d8p+0 + 0x1.8p-53;
      if (x == 0x1.fffffffffff70p-1)
	return 0x1.8000000000009p-23 + 0x1p-77;
      if (x == 0x1.390e6939cd1a6p-5)
	return 0x1.8856a5d3296a4p+0 - 0x1p-109;
      uint64_t w = asuint64 (ps);
      if ((w ^ t) >> 63)
	t--;
      else
	t++;
      pl = asdouble (t);
    }
  return ph + pl;
}

#ifndef __ieee754_acos
libm_alias_finite (__ieee754_acos, __acos)
#endif
