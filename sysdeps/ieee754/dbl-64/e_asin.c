/* Correctly-rounded binary64 arcsine function.

Copyright (c) 2022-2026 Alexei Sibidanov <sibid@uvic.ca>.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/asin/asin.c, revision 646e6ce4).

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
#include "e_asin_data.h"

static inline double
fasttwosum (double x, double y, double *e)
{
  double s = x + y, z = s - x;
  *e = y - z;
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
muldd (double xh, double xl, double ch, double cl, double *l)
{
  double ahhh = ch * xh;
  *l = (cl * xh + ch * xl) + fma (ch, xh, -ahhh);
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

static double __attribute__ ((noinline, cold)) as_asin_refine (double, double);
static double __attribute__ ((noinline, cold)) as_asin_database (double,
								 double);

double
__ieee754_asin (double x)
{
  // coefficients of a polynomial approximation of asin(x):
  // asin(x) = x*(cc[j][0] + cc[j][1] + t*P(t, cc[j] + 2))
  // where t = x^2 - j/128

  uint64_t ix = asuint64 (x);
  uint64_t ax = ix << 1;
  double t, z, zl, jd, f0h, f0l, eps;
  /* exhaustive search done for 2^-4 <= x < 1 with and without FMA
     contraction */
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
      // for |x|>0.5 we use range reduction for double angle formula
      // asin(x) = pi/2 - 2*asin(sqrt((1-x)/2)) and for x<-0.5 acos(x) = -pi/2
      // + 2*asin(sqrt((1-|x|)/2))
      t = 2 - 2 * fabs (x);
      jd = roundeven_finite (t * 0x1p5);
      z = copysign (sqrt (t), -x);
      zl = fma (z, z, -t) * ((-0.5 / t) * z);
      t = 0.25 * t - jd * 0x1p-7;
      // fails with 0x1.98p-52 and x=0x1.3f47056fc030ap-1 (rndz, no fma)
      eps = fabs (z * t) * 0x1.99p-52;
    }
  else
    { // |x|<=0.5
      // for |x| < 0x1.7137449123ef6p-26 |asin(x) - x| is less than half of ulp
      // of asin(x)
      if (__glibc_unlikely (ax < UINT64_C (0x7cae26e892247dec)))
	{
	  return __math_check_uflow_zero_lt (x, 0x1p-1022,
					     fma (0x1p-55, x, x)); // underflow
	}
      f0h = 0;
      f0l = 0;
      t = x * x;
      jd = roundeven_finite (t * 0x1p7);
      t = fma (x, x, -0x1p-7 * jd);
      z = x;
      zl = 0;
      // fails for 0x1.0fp-52 with x=0x1.fa3c79a3c19abp-3 (rndz, no FMA)
      eps = fabs (z * t) * 0x1.10p-52;
    }
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
    return as_asin_refine (x, lb);
  return lb;
}

double
as_asin_refine (double x, double phi)
{
  // Consider x as sin(phi) then cos(phi) is ch + cl = sqrt(1-x^2)
  // Using angle rotation formula bring the argument close to zero
  // where the asin Taylor expansion works well.
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

  int64_t jf = roundeven_finite (fabs (phi) * 0x1.45f306dc9c883p+4);
  // jf = round(|phi|*64/pi)
  // sin(pi/64*j) in the double-double format (smallest term first)
  // this is the same table as in acos.c
  // 0 <= jf <= 32
  double Ch = SINCOS[32 - jf][1], Cl = SINCOS[32 - jf][0], Sh = SINCOS[jf][1],
	 Sl = SINCOS[jf][0];

  double ax = fabs (x);
  double dsh = ax - Sh, dsl = -Sl;
  double dch = ch - Ch, dcl = cl - Cl;

#define MAGIC 0x1.8p-4
  double Sc = fma (Sh, dch, MAGIC) - MAGIC;
  double dSc = fma (Sh, dch, -Sc);

  double Cs = fma (Ch, dsh, MAGIC) - MAGIC;
  double dCs = fma (Ch, dsh, -Cs);

  double v = Cs - Sc;
  double dv = (Ch * dsl + Cl * dsh) - (Sh * dcl + Sl * dch) - (dSc - dCs);
  v = fasttwosum (v, dv, &dv);
  double sgn = copysign (1.0, x), jt = jf * sgn;
  // 0 <= jt <= 64
  double dv2, v2 = muldd (v, dv, v, dv, &dv2);
  v *= sgn;
  dv *= sgn;
  double fl = v2 * (ct[0] + v2 * (ct[1] + v2 * ct[2])),
	 fh = polydd (v2, dv2, 5, POLYC, &fl);
  fh = muldd (v, dv, fh, fl, &fl);

  double ph = jt * 0x1.921fb54442dp-5, pl = 0x1.8469898cc518p-53 * jt,
	 ps = -0x1.fc8f8cbb5bf6cp-102 * jt;

  // since 0 <= jt <= 64, ph and pl are exact
  pl = fastsum (fh, fl, pl, ps, &ps);
  ph = fasttwosum (ph, pl, &pl);
  pl = fasttwosum (pl, ps, &ps);
  ph = fasttwosum (ph, pl, &pl);
  pl = fasttwosum (pl, ps, &ps);

  uint64_t th = asuint64 (ph), tl = asuint64 (pl);
  uint64_t tn = (th & (UINT64_C (0x7ff) << 52)) - (UINT64_C (53) << 52);
  tl &= ~UINT64_C (0) >> 1;
  long dn = tl - tn, de = (tn - tl) >> 52;
  // dn=-2 for x=0x1.c373ff4aad79bp-14 (rndn)
  // de=48 for x=0x1.da4e0e6c717a5p-2 (rndz)
  int hard = (-2 <= dn && dn <= 0) || (de > 47);
  double res = ph + pl;
  if (hard)
    res = as_asin_database (x, res);
  return res;
}

double
as_asin_database (double x, double f)
{
  // db[] is 64-bit encoding of |x| for x exceptional cases
  // sorted by increasing first values
  // those marked with * are required only without FMA contraction
  double ax = fabs (x);
  int a = 0, b = sizeof (db) / sizeof (db[0]);
  while (a + 1 < b)
    { // binary search with invariant db[a][0] <= x < db[b][0]
      int m = (a + b) / 2;
      if (db[m][0] <= ax)
	a = m;
      else
	b = m;
    }
  return (db[a][0] == ax)
	     ? (x > 0) ? db[a][1] + db[a][2] : -db[a][1] - db[a][2]
	     : f;
}

#ifndef __ieee754_asin
libm_alias_finite (__ieee754_asin, __asin)
#endif
