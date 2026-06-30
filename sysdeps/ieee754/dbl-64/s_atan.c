/* Correctly-rounded arctangent of binary64 value.

Copyright (c) 2023 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/atan/atan.c, revision 8ea8ea35).

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
#include <libm-alias-double.h>
#include "math_config.h"
#include "s_atan_data.h"

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

static double
polydd (double xh, double xl, int n, const double c[][2], double *l)
{
  int i = n - 1;
  double ch = c[i][0] + *l, cl = (c[i][0] - ch) + *l + c[i][1];
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

// this table contains 129 entries

// this routine might be called for 0x1p-27 <= |x|
// a is the approximation of atan(x) from the fast path
// thus 0x1p-27 <= |a| <= pi/2
static double __attribute__ ((cold, noinline))
as_atan_refine2 (double x, double a)
{
  static const double ch[][2]
      = { { -0x1.5555555555555p-2, -0x1.5555555555555p-56 },
	  { 0x1.999999999999ap-3, -0x1.999999999bcb8p-57 },
	  { -0x1.2492492492492p-3, -0x1.249242093c016p-57 } };
  static const double cl[] = { 0x1.c71c71c71c71cp-4, -0x1.745d1745d1265p-4,
			       0x1.3b13b115bcbc4p-4, -0x1.1107c41ad3253p-4 };
  uint64_t phi = asuint64 (fabs (a) * 0x1.45f306dc9c883p6 + 256.5);
  // 256.5 < phi < 384.5
  int64_t i = (phi >> (52 - 8)) & 0xff; // 0 <= i <= 128
  double h, hl;
  if (i == 128)
    {
      h = -1.0 / x;
      hl = fma (h, x, 1) * h;
    }
  else
    {
      double ta = copysign (A[i][0], x);
      double zta = x * ta, ztal = fma (x, ta, -zta), zmta = x - ta;
      double v = 1 + zta, d = 1 - v, ev = (d + zta) - ((d + v) - 1) + ztal;
      double r = 1.0 / v, rl = (fma (r, -v, 1.0) - ev * r) * r;
      h = r * zmta;
      hl = fma (r, zmta, -h) + rl * zmta;
    }
  double h2l, h2 = muldd_acc (h, hl, h, hl, &h2l), h4 = h2 * h2;
  double h3l, h3 = muldd_acc (h, hl, h2, h2l, &h3l);
  double fl = h2 * ((cl[0] + h2 * cl[1]) + h4 * (cl[2] + h2 * cl[3])),
	 f = polydd (h2, h2l, 3, ch, &fl);
  f = muldd_acc (h3, h3l, f, fl, &fl);
  double ah, al, at;
  if (i == 0)
    {
      ah = h;
      al = f;
      at = fl;
    }
  else
    {
      double df = 0;
      if (i < 128)
	df = copysign (1.0, x) * A[i][1];
      double id = copysign (i, x);
      ah = 0x1.921fb54442dp-7 * id;
      al = 0x1.8469898cc518p-55 * id, at = -0x1.fc8f8cbb5bf8p-104 * id;
      al = adddd (al, at, df, 0, &at);
      al = adddd (al, at, h, hl, &at);
      al = adddd (al, at, f, fl, &at);
    }
  double v2, v0 = fasttwosum (ah, al, &v2), v1 = fasttwosum (v2, at, &v2);
  double ax = fabs (x);
  uint64_t t0 = asuint64 (v0), t1 = asuint64 (v1);
  if (__glibc_unlikely (((t1 + 1) & (~UINT64_C (0) >> 12)) <= 2
			|| ((t0 >> 52) & 0x7ff) - ((t1 >> 52) & 0x7ff) > 103))
    {
      for (unsigned j = 0; j < sizeof (db) / sizeof (db[0]); j++)
	if (ax == db[j][0])
	  return copysign (db[j][1], x) + copysign (1.0, x) * db[j][2];
      if (!(t1 & (~UINT64_C (0) >> 12)))
	{
	  uint64_t w = asuint64 (v2);
	  if ((w ^ t1) >> 63)
	    t1--;
	  else
	    t1++;
	  v1 = asdouble (t1);
	}
    }
  return v1 + v0;
}

double
__atan (double x)
{
  static const double ch[] = { 0x1p+0, -0x1.555555555552bp-2,
			       0x1.9999999069c2p-3, -0x1.248d2c8444ac6p-3 };
  uint64_t t = asuint64 (x);
  uint64_t at = t & (~UINT64_C (0) >> 1); // at encodes |x|
  int64_t i = (at >> 51) - 2030l;	  // -2030 <= i <= 2065
  if (__glibc_unlikely (at < UINT64_C (0x3f7b21c475e6362a)))
    {
      // |x| < 0x1.b21c475e6362ap-8
      if (__glibc_unlikely (at == 0))
	return x; // atan(+/-0) = +/-0
      static const double ch2[]
	  = { -0x1.5555555555555p-2, 0x1.99999999998c1p-3,
	      -0x1.249249176aecp-3, 0x1.c711fd121ae8p-4 };
      if (at < UINT64_C (0x3e40000000000000))
	{ // |x| < 0x1p-27
	  /* We have underflow when 0 < |x| < 2^-1022 or when |x| = 2^-1022
	     and rounding towards zero. */
	  double res = fma (-0x1p-54, x, x);
	  return __math_check_uflow_lt (res, 0x1p-1022);
	}
      double x2 = x * x, x3 = x * x2, x4 = x2 * x2;
      double f = x3 * ((ch2[0] + x2 * ch2[1]) + x4 * (ch2[2] + x2 * ch2[3]));
      double ub = (f + f * 0x4.8p-52) + x, lb = (f - f * 0x2.8p-52) + x;
      if (__glibc_likely (ub == lb))
	return ub;
      return as_atan_refine2 (x, ub);
    }
  double h, ah, al;
  if (__glibc_unlikely (at > UINT64_C (0x4062ded8e34a9035)))
    {
      // |x| > 0x1.2ded8e34a9035p+7
      ah = copysign (0x1.921fb54442d18p+0, x);
      al = copysign (0x1.1a62633145c07p-54, x);
      if (__glibc_unlikely (at >= UINT64_C (0x434d02967c31cdb5)))
	{
	  // |x| >= 0x1.d02967c31cdb5p+53
	  if (__glibc_unlikely (at > (UINT64_C (0x7ff) << 52)))
	    return x + x; // NaN
	  return ah + al;
	}
      h = -1.0 / x;
    }
  else
    {
      // now 0x1.b21c475e6362ap-8 <= |x| <= 0x1.2ded8e34a9035p+7 thus 1<=i<=30
      uint64_t u = t & (~UINT64_C (0) >> 13);
      uint64_t ut = u >> (51 - 16), ut2 = ut * ut >> 16;
      i = (((uint64_t) RTAB[i][0] << 16) + ut * RTAB[i][1] - ut2 * RTAB[i][2])
	  >> (16 + 9);
      double ta = copysign (1.0, x) * A[i][0],
	     id = copysign (1.0, x) * (double) i;
      al = copysign (1.0, x) * A[i][1] + 0x1.8469898cc517p-55 * id;
      h = (x - ta) / (1 + x * ta);
      ah = 0x1.921fb54442dp-7 * id;
    }
  double h2 = h * h, h4 = h2 * h2;
  double f = (ch[0] + h2 * ch[1]) + h4 * (ch[2] + h2 * ch[3]);
  al = fma (h, f, al);
  double e = h * 0x3.fp-52;
  double ub = (al + e) + ah, lb = (al - e) + ah;
  if (__glibc_likely (ub == lb))
    return ub;
  return as_atan_refine2 (x, ub);
}

#ifndef __atan
libm_alias_double (__atan, atan)
#endif
