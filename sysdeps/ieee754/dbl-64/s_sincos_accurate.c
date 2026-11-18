/* Accurate paths of the correctly-rounded binary64 sin, cos, and sincos.

Copyright (c) 2026 Paul Zimmermann and Alexei Sibidanov.

The original version of these routines was copied from the CORE-MATH
project (files src/binary64/sin/sin.c, revision a3fe413f, and
src/binary64/cos/cos.c, revision 9b821fc6).

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

#include <math.h>
#include <math_uint128.h>
#include <stdint.h>
#include "math_config.h"
#include "s_sincos_data.h"

/* Return the correctly rounded cos(x) for the inputs where the accurate
   path result R is too close to a rounding boundary.  */
static __always_inline double
cos_exceptions (double x, double r)
{
  double ax = fabs (x);
  /* cos(x) has 88 identical bits after round bit.  */
  if (ax == 0x1.8000000000009p-23)
    return 0x1.fffffffffff7p-1 + 0x1.b56666666666cp-143;
  /* cos(x) has 82 identical bits after round bit.  */
  if (ax == 0x1.8000000000024p-22)
    return 0x1.ffffffffffdcp-1 + 0x1.b56666666667ep-137;
  /* cos(x) has 76 identical bits after round bit.  */
  if (ax == 0x1.800000000009p-21)
    return 0x1.ffffffffff7p-1 + 0x1.b5666666666c4p-131;
  /* cos(x) has 72 identical bits after round bit.  */
  if (ax == 0x1.20000000000f3p-20)
    return 0x1.fffffffffebcp-1 + 0x1.37642666666fdp-127;
  return r;
}

/* Return the 128-bit number stored as { high, low } in A.  */
static __always_inline u128
u128_load (const uint64_t a[2])
{
  return u128_from_hl (a[0], a[1]);
}

/* Round (-1)^s*r/2^128 to double, assuming r is non-zero and not in the
   subnormal region.  */
static __always_inline double
u128_tod (u128 r, int s)
{
  /* Since the smallest distance from a binary64 number to a multiple of
     pi/2 is 2^-60.888 (see [1]), the smallest value of r/2^128 is about
     2^-60.888 too (taking into account approximation errors), thus
     sh <= 60.  This proves that the high part of r cannot be 0.  */
  unsigned int sh = stdc_leading_zeros (u128_high (r));
  /* Upper 53 non-zero bits.  */
  uint64_t h = u128_low (u128_rshift (r, 75 - sh));
  /* Round bit.  */
  int rbit = u128_low (u128_rshift (r, 74 - sh)) & 1;
  static const double sgn[] = { 0x1p-53, -0x1p-53 };
  /* Scale by 2^-sh.  */
  double v = asdouble (asuint64 (sgn[s]) - ((uint64_t) sh << MANTISSA_WIDTH));
  static const double low[] = { 0x1.8p-2, 0x1.8p-1 };
  double a = h * v, b = low[rbit] * v;
  /* Assume sin(x) > 0, thus s=0.  When rbit is 0, we have b < ulp(a)/2,
     and the result is rounded to a to nearest, which is what we want.
     When rbit is 1, we have b > ulp(a)/2, and the result is rounded to
     nextup(a), which is what we want too.  In both case it is proven in
     sin.pdf that the approximation error cannot make the result cross a
     rounding boundary, except maybe for hard-to-round cases, which are
     checked by sin.wc.  */
  return a + b;
}

/* Return the high 128 bits of the 256-bit product a*b, up to an error of
   at most 2 ulps.  */
static inline u128
mhUU (u128 a, u128 b)
{
  uint64_t ah = u128_high (a), al = u128_low (a);
  uint64_t bh = u128_high (b), bl = u128_low (b);
  u128 ahbh = u128_mul64 (ah, bh);
  u128 ahbl = u128_mul64 (ah, bl);
  u128 albh = u128_mul64 (al, bh);
  return u128_add (ahbh, u128_add (u128_rshift (ahbl, 64),
				   u128_rshift (albh, 64)));
}

/* Return Sr such that Sr/2^128 approximates sin2pi(r), for 0 <= r < 2^-14,
   where u/2^128 approximates r'=8*r, u2/2^128 approximates r'^2,
   u4/2^128 approximates r'^4, and u2h = floor(u2/2^64).  */
static __always_inline u128
evalPS (u128 u, u128 u2, u128 u2h, u128 u4)
{
  /* The computation is performed in fixed point, where each variable a is
     interpreted as a/2^128, thus multiplying two variables a and b means
     taking floor(a*b/2^128).  Since the signs of the coefficients are
     alternating, and each new coefficient dominates the lower terms, the
     lower terms are subtracted from the (absolute value of) the new
     coefficient.  Estrin's scheme is used: the degree-{1,3} part and the
     degree-{5,7} part (multiplied by r^4) are evaluated separately.  */
  u128 sh = u128_sub (u128_load (PS[2]), u128_mul (u128_load (PS[3]), u2h));
  sh = mhUU (sh, u4);
  u128 s = mhUU (u128_load (PS[1]), u2);
  s = u128_sub (u128_load (PS[0]), s);
  s = u128_add (s, sh);
  /* Multiply by r.  */
  return mhUU (s, u);
}

/* Return Cr such that Cr/2^128 approximates cos2pi(r), for 0 <= r < 2^-14,
   where u2/2^128 approximates r'^2, u4/2^128 approximates r'^4, and
   u2h = floor(u2/2^64), with r'=8*r.  */
static __always_inline u128
evalPC (u128 u2, u128 u2h, u128 u4)
{
  /* Estrin's scheme.  */
  u128 sh = u128_sub (u128_load (PC[3]), u128_mul (u2h, u128_load (PC[4])));
  sh = u128_sub (u128_load (PC[2]), mhUU (sh, u2));
  u128 s = mhUU (u128_load (PC[1]), u2);
  s = u128_sub (u128_load (PC[0]), s);
  return u128_add (s, mhUU (sh, u4));
}

/* Argument reduction for the accurate path: return k and r such that
   x/(2pi) mod 1 = k/2^13 + r/2^128 + eps with |r/2^128| <= 2^-14 and
   0 <= eps < 2^-128 + 2^-139 < 2^-127.999.  NEG is set to 0 if r >= 0,
   and to 1 if r < 0.  */
static __always_inline uint64_t
reduce_large_acc (u128 *r, int *neg, double x)
{
  uint64_t t = asuint64 (x);
  /* 996 <= e <= 2046.  */
  int e = (t >> MANTISSA_WIDTH) & 0x7ff;
  uint64_t m = (UINT64_C(1) << MANTISSA_WIDTH) | get_mantissa (t);
  /* x = m * 2^(e-1075).  T[j] corresponds to T[j]/2^(64*j) thus T[j]*x
     corresponds to m*T[j]*2^(e-1075-64*j).  To get a non-zero fractional
     value, we need e-1075-64*j < 0, thus 64*j > e-1075 or 64*j >= e-1074.
     i = ceil((e-1074)/64), -1 <= i <= 16.  */
  int i = -1 + (e - 947) / 64;
  int f = (e - 1011) & 0x3f;
  /* Shift the table entries.  */
  uint64_t v0 = (i >= 0) ? T[i] : 0, v1 = T[i + 1], v2 = T[i + 2];
  if (f != 0)
    {
      v0 = (v0 << f) | (v1 >> (64 - f));
      v1 = (v1 << f) | (v2 >> (64 - f));
      v2 = (v2 << f) | (T[i + 3] >> (64 - f));
    }
  /* m*v0 contributes to 64 bits to the fractional part of x/(2pi), from
     weight 2^-1 to 2^-64, m*v1 contributes to 53+64 bits from weight 2^-12
     to 2^-128, m*v2 contributes to 53+64 bits from weight 2^-76 to 2^-192,
     the ignored part m*(v3+v4+...) contributes to less than 2^-139.  */
  u128 u = u128_mul (u128_from_u64 (m), u128_from_hl (v0, v1));
  u128 v = u128_mul64 (m, v2);
  /* Add the contribution of m*v2 from 2^-76 to 2^-128, the ignored part of
     v contributes to less than 2^-128.  */
  u = u128_add (u, u128_rshift (v, 64));
  uint64_t k = u128_high (u) >> (64 - 13);
  /* Ignore the leading 13 bits.  */
  u = u128_from_hl (u128_high (u) & UINT64_C(0x7ffffffffffff), u128_low (u));
  /* Round k to nearest to have |r/2^128| < 2^-14.  */
  *neg = u128_high (u) >> 50;
  if (*neg)
    {
      /* Add 1 to k and subtract 1/2^13 to r.  */
      k = (k + 1) & ((UINT64_C(1) << 13) - 1);
      u = u128_sub (u128_from_hl (UINT64_C(1) << 51, 0), u);
    }
  *r = u;
  return k;
}

/* Accurate path for |x| >= 2^-16.  */
double
__sin_large_accurate (double x)
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

/* Accurate path for x >= 2^-27.  */
double
__cos_large_accurate (double x)
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
       cos(x) ~ cos(pi*k/2^12 + 2*pi*r)
	      ~ cos(pi*k/2^12)*cos(2*pi*r) - sin(pi*k/2^12)*sin(2*pi*r)
     Write k = 2^12*s + 2^6*i1 + i2 and t1=pi*i1/2^6, t2 = pi*i2/2^12
     then sin(pi*k/2^12) = (-1)^s*[sin(t1)*cos(t2)+cos(t1)*sin(t2)]
     and  cos(pi*k/2^12) = (-1)^s*[cos(t1)*cos(t2)-sin(t1)*sin(t2)].  */
  int sbit = (k >> 12) & 1;
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

  /* Now combine: cos(x) ~ c1*C - s1*S.  */
  c1u = mhUU (c1u, Cr);
  s1u = mhUU (s1u, Sr);

  /* c1u/2^128 approximates cos(z)*cos(r) where cos(z) > 0 for i1 < 32,
     and cos(z) <= 0 for i1 >= 32; s1u/2^128 approximates sin(z)*sin(r),
     which has the sign of r.  */
  if ((i1 >= 32) ^ neg)
    /* cos(z) * r < 0.  */
    c1u = u128_add (c1u, s1u);
  else if (!u128_lt (s1u, c1u))
    {
      c1u = u128_sub (s1u, c1u);
      sbit ^= 1;
    }
  else
    c1u = u128_sub (c1u, s1u);
  /* If i1 >= 32, cos(z) <= 0 thus the signs should be exchanged.  */
  double ret = u128_tod (c1u, sbit ^ (i1 >= 32));
  uint64_t d = u128_low (c1u) + 5;
  if (d <= 3)
    return cos_exceptions (x, ret);
  return ret;
}
