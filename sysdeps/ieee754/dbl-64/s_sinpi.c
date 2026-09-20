/* Correctly-rounded sine of binary64 value for angles in half-revolutions.

Copyright (c) 2023-2025 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/sinpi/sinpi.c, revision cd0b9d90).

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
#include <libm-alias-double.h>
#include "math_config.h"
#include "s_sincospi_data.h"

static inline double
fasttwosum (double x, double y, double *e)
{
  double s = x + y, z = s - x;
  *e = y - z;
  return s;
}

static inline double
adddd (double xh, double xl, double ch, double cl, double *l)
{
  double s = xh + ch, d = s - xh;
  *l = ((ch - d) + (xh + (d - s))) + (xl + cl);
  return s;
}

static inline double
muldd_acc (double xh, double xl, double ch, double cl, double *l)
{
  double ahlh = ch * xl, alhh = cl * xh, ahhh = ch * xh,
	 ahhl = fma (ch, xh, -ahhh);
  ahhl += alhh + ahlh;
  return fasttwosum (ahhh, ahhl, l);
}

static inline double
mulddd (double xh, double xl, double ch, double *l)
{
  double ahlh = ch * xl, ahhh = ch * xh, ahhl = fma (ch, xh, -ahhh);
  ahhl += ahlh;
  ch = ahhh + ahhl;
  *l = (ahhh - ch) + ahhl;
  return ch;
}

static inline double
polydd (double xh, double xl, int n, const double c[][2], double *l)
{
  int i = n - 1;
  double ch = c[i][0] + *l, cl = ((c[i][0] - ch) + *l) + c[i][1];
  while (--i >= 0)
    {
      ch = muldd_acc (xh, xl, ch, cl, &cl);
      double th = ch + c[i][0], tl = (c[i][0] - th) + ch;
      ch = th;
      cl += tl + c[i][1];
    }
  *l = cl;
  return ch;
}

/* Accurate path for |x| <= 0x1.2p-5, where sinpi(x) = pi*x*(1 + O(x^2)).  */
static double
as_sinpi_zero (double x)
{
  double x2 = x * x, dx2 = fma (x, x, -x2);
  double x3 = x2 * x, dx3 = fma (x2, x, -x3) + dx2 * x;
  static const double ch[][2] =
    {
      { -0x1.4abbce625be53p+2, 0x1.05511c68477bep-52 },
      { 0x1.466bc6775aae2p+1, -0x1.6dc0cbefae1dap-54 },
      { -0x1.32d2cce62bd86p-1, 0x1.066bd54973829p-55 },
      { 0x1.50783487ee781p-4, 0x1.832989f39a743p-58 }
    };
  static const double cl[3] =
    {
      -0x1.e3074fde861fp-8, 0x1.e8f4344534da6p-12, -0x1.6f9cd7b8cb9dbp-16
    };
  double fl = x2 * (cl[0] + x2 * (cl[1] + x2 * cl[2]));
  double fh = polydd (x2, dx2, 4, ch, &fl);
  fh = muldd_acc (fh, fl, x3, dx3, &fl);
  const double pi0 = 0x1.92p+1, pi1 = 0x1.fb54442d1846ap-11,
	       pi2 = -0x1.d9cceba3f91f2p-65;
  double y0 = pi0 * x;
  uint64_t b = asuint64 (y0) & MANTISSA_MASK;
  b += (uint64_t) 85 << 51;
  double bf = asdouble (b);
  y0 = (y0 + bf) - bf;
  double y0l = fma (pi0, x, -y0);
  double y1 = pi1 * x, y2 = fma (pi1, x, -y1) + pi2 * x;
  y1 = adddd (y1, y2, y0l, 0, &y2);
  y1 = adddd (y1, y2, fh, fl, &y2);
  y0 = fasttwosum (y0, y1, &y1);
  y1 = fasttwosum (y1, y2, &y2);
  uint64_t t = asuint64 (y1);
  if (__glibc_unlikely (!(t & MANTISSA_MASK)))
    {
      if ((asuint64 (y2) ^ t) >> 63)
	t--;
      else
	t++;
      y1 = asdouble (t);
    }
  return y0 + y1;
}

/* Accurate path: IQ is the index of the reduced argument in the sincosn
   table, Z the scaled remainder.  */
static double
as_sinpi_refine (int iq, double z)
{
  double x = z * 0x1p-63, x2 = x * x, dx2 = fma (x, x, -x2);
  static const double sh[][2] =
    {
      { 0x1.921fb54442d18p+1, 0x1.1a62633145c06p-53 },
      { -0x1.4abbce625be53p-22, 0x1.05511cbc65743p-76 },
      { 0x1.466bc6775aae1p-47, -0x1.9c3c168d990ap-114 }
    };
  static const double ch[][2] =
    {
      { -0x1.3bd3cc9be45dep-22, -0x1.692b71366cc04p-76 },
      { 0x1.03c1f081b5ac4p-46, -0x1.32b33fda9113cp-100 }
    };
  double sll = -0x1.32d2cc920dcb4p-73 * x2;
  double slh = polydd (x2, dx2, 3, sh, &sll);
  slh = mulddd (slh, sll, x * 0x1p-12, &sll);
  double cll = x2 * (-0x1.55d3c7e3cbff9p-72 + 0x1.e1f50604fa0ffp-99 * x2);
  double clh = polydd (x2, dx2, 2, ch, &cll);
  clh = muldd_acc (clh, cll, x2, dx2, &cll);
  double sbh, sbl, cbh, cbl;
  __sincospi_sincosn2 (iq, &sbh, &sbl, &cbh, &cbl);
  double csl, csh = muldd_acc (clh, cll, sbh, sbl, &csl);
  double scl, sch = muldd_acc (slh, sll, cbh, cbl, &scl);
  double tsl, tsh = fasttwosum (sch, csh, &tsl);
  tsl += csl + scl;
  double tsl2;
  tsh = fasttwosum (sbh, tsh, &tsl2);
  tsl = sbl + tsl + tsl2;
  uint64_t t = asuint64 (tsl);
  if ((t | ((uint64_t) 0xfff << 52)) == ~(uint64_t) 0 || (t << 12) == 0)
    {
      static const struct { int iq; double x, r, d; } db[] =
	{
	  { 76, -0x1.276b3fef466p-2, 0x1.db8a79a80c3a0p-4, 0x1p-110 },
	  { 108, -0x1.33caea0f24cp-2, 0x1.5146c0bc45bcep-3, 0x1p-109 },
	  { 490, -0x1.8a1e8a3e82cp-1, 0x1.5d6561936b699p-1, -0x1p-55 },
	  { 903, -0x1.bdd02d1ad60p-2, 0x1.f72c906962631p-1, 0x1p-55 },
	};
      double sgn = iq > 2048 ? -1 : 1;
      iq &= 0x7ff;
      for (unsigned int i = 0; i < array_length (db); i++)
	if ((x == db[i].x && iq == db[i].iq)
	    || (x == -db[i].x && iq == 2048 - db[i].iq))
	  return sgn * db[i].r + sgn * db[i].d;
    }
  return tsh + tsl;
}

double
__sinpi (double x)
{
  static const double sn[] =
    {
      0x1.921fb54442d18p-74, -0x1.4abbce625be51p-223, 0x1.466bc6044ba16p-374
    };
  static const double cn[] =
    {
      -0x1.3bd3cc9be45dbp-148, 0x1.03c1f00186416p-298
    };
  uint64_t ix = asuint64 (x);
  uint64_t ax = ix & EXP_MANT_MASK;
  if (__glibc_unlikely (ax == 0))
    return x;
  int32_t e = ax >> MANTISSA_WIDTH;
  uint64_t m0 = (ix & MANTISSA_MASK) | ((uint64_t) 1 << MANTISSA_WIDTH);
  int64_t sgn = ix;
  sgn >>= 63;
  int64_t m = ((int64_t) m0 ^ sgn) - sgn;
  int32_t s = 1063 - e;
  if (__glibc_unlikely (s < 0))
    { /* |x| >= 2^41 */
      if (__glibc_unlikely (e == 0x7ff))
	{
	  if (!(ix << 12))
	    return __math_invalid (x); /* x = +-Inf */
	  return x + x;		     /* x = NaN */
	}
      s = -s - 1;
      if (s > 10)
	return copysign (0.0, x);
      uint64_t iq = (uint64_t) m << s;
      if (!(iq & 2047))
	return copysign (0.0, x);
      double sh, sl, ch, cl;
      __sincospi_sincosn (iq, &sh, &sl, &ch, &cl);
      return sh + sl;
    }

  if (__glibc_unlikely (ax <= 0x3fa2000000000000ull))
    { /* |x| <= 0x1.2p-5 */
      double ph = 0x1.921fb54442d18p+1, pl = 0x1.1a62633145c07p-53;
      double zh, zl;
      if (__glibc_unlikely (fabs (x) < 0x1p-54))
	{
	  /* Here sinpi(x) = pi*x rounded once.  Evaluate the product scaled
	     by 2^106 and scale the result back: computing it directly would
	     raise a spurious underflow, either because pl * x is subnormal
	     (for |x| slightly above 2^-970) or because the low part cancels
	     into the subnormal range.  */
	  double t = x * 0x1p106;
	  zh = ph * t;
	  zl = fma (ph, t, -zh) + pl * t;
	  double r = zh + zl, rs = r * 0x1p-106, rt = rs * 0x1p106;
	  double res = fma ((zh - rt) + zl, 0x1p-106, rs);
	  /* For all rounding modes there is underflow (before or after
	     rounding) for |x| <= 0x1.45f306dc9c882p-1024.  */
	  if (fabs (x) <= 0x1.45f306dc9c882p-1024)
	    return __math_erange (res);
	  return res;
	}
      zh = ph * x;
      zl = fma (ph, x, -zh) + pl * x;
      double x2 = x * x, x3 = x2 * x, x4 = x2 * x2;
      double eps = x * (x2 * 0x1p-47 + 0x1p-102);
      static const double c[] =
	{
	  -0x1.4abbce625be51p+2, 0x1.466bc67754b46p+1, -0x1.32d2cc12a51f4p-1,
	  0x1.5060540058476p-4
	};
      zl += x3 * ((c[0] + x2 * c[1]) + x4 * (c[2] + x2 * c[3]));
      double lb = (zl - eps) + zh, ub = (zl + eps) + zh;
      if (lb == ub)
	return lb;
      return as_sinpi_zero (x);
    }

  int32_t si = e - 1011;
  if (__glibc_unlikely (si >= 0 && (m0 << (si + 1)) == 0))
    { /* x is integer or half-integer */
      if ((m0 << si) == 0)
	return copysign (0.0, x); /* x is integer */
      int t = (m0 << (si - 1)) >> 63;
      /* t = 0 if |x| = 1/2 mod 2, t = 1 if |x| = 3/2 mod 2.  */
      return (t == 0) ? copysign (1.0, x) : -copysign (1.0, x);
    }

  uint64_t iq = (m >> s) & 8191;
  iq = (iq + 1) >> 1;
  int64_t k = (uint64_t) m << (e - 1000);
  double z = k, z2 = z * z;
  double fs = sn[0] + z2 * (sn[1] + z2 * sn[2]);
  double fc = cn[0] + z2 * cn[1];
  double sh, sl, ch, cl;
  __sincospi_sincosn (iq, &sh, &sl, &ch, &cl);
  double er = fabs (z) * 0x1p-123 + 0x1p-78;
  double r = sl + sh * (z2 * fc) + ch * (z * fs);
  double lb = (r - er) + sh, ub = (r + er) + sh;
  if (__glibc_likely (lb == ub))
    return lb;
  return as_sinpi_refine (iq, z);
}
libm_alias_double (__sinpi, sinpi)
