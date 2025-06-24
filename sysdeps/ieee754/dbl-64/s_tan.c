/* Correctly-rounded tangent function for binary64 value.

Copyright (c) 2022-2025 Paul Zimmermann and Tom Hubrecht

This file is part of the CORE-MATH project
(https://core-math.gitlabpages.inria.fr/).

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
#include <fenv.h>
#include <errno.h>
#include <libm-alias-double.h>
#include "s_sincos_data.h"

#ifndef SECTION
# define SECTION
#endif

typedef union {double f; uint64_t u;} b64u64_u;

// Multiply exactly a and b, such that *hi + *lo = a * b. 
static inline void a_mul(double *hi, double *lo, double a, double b) {
  *hi = a * b;
  *lo = __builtin_fma (a, b, -*hi);
}

/* Multiply a double with a double double : a * (bh + bl)
   with error bounded by ulp(lo) */
static inline void s_mul (double *hi, double *lo, double a, double bh,
                          double bl) {
  a_mul (hi, lo, a, bh); /* exact */
  *lo = __builtin_fma (a, bl, *lo);
  /* the error is bounded by ulp(lo), where |lo| < |a*bl| + ulp(hi) */
}

// Returns (ah + al) * (bh + bl) - (al * bl)
// We can ignore al * bl when assuming al <= ulp(ah) and bl <= ulp(bh)
static inline void d_mul(double *hi, double *lo, double ah, double al,
                         double bh, double bl) {
  double s, t;

  a_mul(hi, &s, ah, bh);
  t = __builtin_fma(al, bh, s);
  *lo = __builtin_fma(ah, bl, t);
}

static inline void
fast_two_sum(double *hi, double *lo, double a, double b)
{
  double e;

  *hi = a + b;
  e = *hi - a; /* exact */
  *lo = b - e; /* exact */
}

/* Put in h+l an approximation of sin2pi(xh+xl),
   for -2^-24 <= xh+xl < 2^-11 + 2^-24,
   and |xl| < 2^-52*|xh|, with relative error < 2^-71.61
   (see evalPSfast_all(K=8) in tan.sage).
   Assume uh + ul approximates (xh+xl)^2. */
static void
evalPSfast (double *h, double *l, double xh, double xl, double uh, double ul)
{
  double t;
  *h = PSfast[4]; // degree 7
  *h = __builtin_fma (*h, uh, PSfast[3]); // degree 5
  *h = __builtin_fma (*h, uh, PSfast[2]); // degree 3
  s_mul (h, l, *h, uh, ul);
  fast_two_sum (h, &t, PSfast[0], *h);
  *l += PSfast[1] + t;
  // multiply by xh+xl
  d_mul (h, l, *h, *l, xh, xl);
}

/* Put in h+l an approximation of cos2pi(xh+xl),
   for -2^-24 <= xh+xl < 2^-11 + 2^-24,
   and |xl| < 2^-52.36, with relative error < 2^-69.96
   (see evalPCfast(rel=true) in tan.sage).
   Assume uh + ul approximates (xh+xl)^2. */
static void
evalPCfast (double *h, double *l, double uh, double ul)
{
  double t;
  *h = PCfast[4]; // degree 6
  *h = __builtin_fma (*h, uh, PCfast[3]); // degree 4
  *h = __builtin_fma (*h, uh, PCfast[2]); // degree 2
  s_mul (h, l, *h, uh, ul);
  fast_two_sum (h, &t, PCfast[0], *h);
  *l += PCfast[1] + t;
}

/* Put in Y an approximation of sin2pi(X), for 0 <= X < 2^-11,
   where X2 approximates X^2.
   Absolute error bounded by 2^-132.999 with 0 <= Y < 0.003068
   (see evalPS() in sin.sage), and relative error bounded by
   2^-124.648 (see evalPSrel(K=8) in sin.sage). */
static void
evalPS (dint64_t *Y, dint64_t *X, dint64_t *X2)
{
  mul_dint_21 (Y, X2, PS+5); // degree 11
  add_dint (Y, Y, PS+4);     // degree 9
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PS+3);     // degree 7
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PS+2);     // degree 5
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PS+1);     // degree 3
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PS+0);     // degree 1
  mul_dint (Y, Y, X);        // multiply by X
}

/* Put in Y an approximation of cos2pi(X), for 0 <= X < 2^-11,
   where X2 approximates X^2.
   Absolute/relative error bounded by 2^-125.999 with 0.999995 < Y <= 1
   (see evalPC() in sin.sage). */
static void
evalPC (dint64_t *Y, dint64_t *X2)
{
  mul_dint_21 (Y, X2, PC+5); // degree 10
  add_dint (Y, Y, PC+4);     // degree 8
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PC+3);     // degree 6
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PC+2);     // degree 4
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PC+1);     // degree 2
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PC+0);     // degree 0
}

// normalize X such that X->hi has its most significant bit set (if X <> 0)
static void
normalize (dint64_t *X)
{
  int cnt;
  if (X->hi != 0)
  {
    cnt = __builtin_clzll (X->hi);
    if (cnt)
    {
      X->hi = (X->hi << cnt) | (X->lo >> (64 - cnt));
      X->lo = X->lo << cnt;
    }
    X->ex -= cnt;
  }
  else if (X->lo != 0)
  {
    cnt = __builtin_clzll (X->lo);
    X->hi = X->lo << cnt;
    X->lo = 0;
    X->ex -= 64 + cnt;
  }
}

/* Approximate X/(2pi) mod 1. If Xin is the input value, and Xout the
   output value, we have:
   |Xout - (Xin/(2pi) mod 1)| < 2^-126.67*|Xout|
   Assert X is normalized at input, and normalize X at output.
*/
static void
reduce (dint64_t *X)
{
  int e = X->ex;
  u128 u;

  if (e <= 1) // |X| < 2
  {
    /* multiply by T[0]/2^64 + T[1]/2^128, where
       |T[0]/2^64 + T[1]/2^128 - 1/(2pi)| < 2^-130.22 */
    u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[1]));
    uint64_t tiny = u128_low (u);
    X->lo = u128_high (u);
    u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[0]));
    X->lo += u128_low (u);
    X->hi = u128_low (u128_add (u128_rshift (u, 64),
				u128_from_u64 (X->lo < u128_low (u))));
    /* hi + lo/2^64 + tiny/2^128 = hi_in * (T[0]/2^64 + T[1]/2^128) thus
       |hi + lo/2^64 + tiny/2^128 - hi_in/(2*pi)| < hi_in * 2^-130.22
       Since X is normalized at input, hi_in >= 2^63, and since T[0] >= 2^61,
       we have hi >= 2^(63+61-64) = 2^60, thus the normalize() below
       perform a left shift by at most 3 bits */
    e = X->ex;
    normalize (X);
    e = e - X->ex;
    // put the upper e bits of tiny into X->lo
    if (e)
      X->lo |= tiny >> (64 - e);
    /* The error is bounded by 2^-130.22 (relative) + ulp(lo) (absolute).
       Since now X->hi >= 2^63, the absolute error of ulp(lo) converts into
       a relative error of less than 2^-127.
       This yields a maximal relative error of:
       (1 + 2^-130.22) * (1 + 2^-127) - 1 < 2^-126.852.
    */
    return;
  }

  // now 2 <= e <= 1024

  /* The upper 64-bit word X->hi corresponds to hi/2^64*2^e, if multiplied by
     T[i]/2^((i+1)*64) it yields hi*T[i]/2^128 * 2^(e-i*64).
     If e-64i <= -128, it contributes to less than 2^-128;
     if e-64i >= 128, it yields an integer, which is 0 modulo 1.
     We thus only consider the values of i such that -127 <= e-64i <= 127,
     i.e., (-127+e)/64 <= i <= (127+e)/64.
     Up to 4 consecutive values of T[i] can contribute (only 3 when e is a
     multiple of 64). */
  int i = (e < 127) ? 0 : (e - 127 + 64 - 1) / 64; // ceil((e-127)/64)
  // 0 <= i <= 15
  uint64_t c[5];
  u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[i+3]));
  c[0] = u128_low (u);
  c[1] = u128_high (u);
  u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[i+2]));
  c[1] += u128_low (u);
  c[2] = u128_low (u128_add (u128_rshift (u, 64),
			     u128_from_u64 (c[1] < u128_low (u))));
  u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[i+1]));
  c[2] += u128_low (u);
  c[3] = u128_low (u128_add (u128_rshift (u, 64),
			     u128_from_u64 (c[2] < u128_low (u))));
  u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[i]));
  c[3] += u128_low (u);
  c[4] = u128_low (u128_add (u128_rshift (u, 64),
			     u128_from_u64 (c[3] < u128_low (u))));

  /* up to here, the ignored part hi*(T[i+4]+T[i+5]+...) can contribute by
     less than 2^64 in c[0], thus less than 1 in c[1] */

  int f = e - 64 * i; // hi*T[i]/2^128 is multiplied by 2^f
  /* {c, 5} = hi*(T[i]+T[i+1]/2^64+T[i+2]/2^128+T[i+3]/2^192) */
  /* now shift c[0..4] by f bits to the left */
  uint64_t tiny;
  if (f < 64)
  {
    X->hi = (c[4] << f) | (c[3] >> (64 - f));
    X->lo = (c[3] << f) | (c[2] >> (64 - f));
    tiny = (c[2] << f) | (c[1] >> (64 - f));
    /* the ignored part was less than 1 in c[1],
       thus less than 2^(f-64) <= 1/2 in tiny */
  }
  else if (f == 64)
  {
    X->hi = c[3];
    X->lo = c[2];
    tiny = c[1];
    /* the ignored part was less than 1 in c[1],
       thus less than 1 in tiny */
  }
  else /* 65 <= f <= 127: this case can only occur when e >= 65 */
  {
    int g = f - 64; /* 1 <= g <= 63 */
    /* we compute an extra term */
    u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[i+4]));
    u = u128_rshift (u, 64);
    c[0] += u128_low (u);
    c[1] += (c[0] < u128_low (u));
    c[2] += (c[0] < u128_low (u)) && c[1] == 0;
    c[3] += (c[0] < u128_low (u)) && c[1] == 0 && c[2] == 0;
    c[4] += (c[0] < u128_low (u)) && c[1] == 0 && c[2] == 0 && c[3] == 0;
    X->hi = (c[3] << g) | (c[2] >> (64 - g));
    X->lo = (c[2] << g) | (c[1] >> (64 - g));
    tiny = (c[1] << g) | (c[0] >> (64 - g));
    /* the ignored part was less than 1 in c[0],
       thus less than 1/2 in tiny */
  }
  /* The approximation error between X/in(2pi) mod 1 and
     X->hi/2^64 + X->lo/2^128 + tiny/2^192 is:
     (a) the ignored part in tiny, which is less than ulp(tiny),
         thus less than 1/2^192;
     (b) the ignored terms hi*T[i+4] + ... or hi*T[i+5] + ...,
         which accumulate to less than ulp(tiny) too, thus
         less than 1/2^192.
     Thus the approximation error is less than 2^-191 (absolute).
  */
  X->ex = 0;
  normalize (X);
  /* the worst case (for 2^25 <= x < 2^1024) is X->ex = -61, attained
     for |x| = 0x1.6ac5b262ca1ffp+851 */
  if (X->ex < 0) // put the upper -ex bits of tiny into low bits of lo
    X->lo |= tiny >> (64 + X->ex);
  /* Since X->ex >= -61, it means X >= 2^-62 before the normalization,
     thus the maximal absolute error of 2^-191 yields a relative error
     bounded by 2^-191/2^-62 = 2^-129.
     There is an additional truncation error (for tiny) of at most 1 ulp
     of X->lo, thus at most 2^-127.
     The relative error is thus bounded by 2^-126.67. */
}

/* Given Xin:=X with 0 <= Xin < 1, return i and modify X such that
   Xin = i/2^11 + Xout, with 0 <= Xout < 2^-11.
   This operation is exact. */
static int
reduce2 (dint64_t *X)
{
  if (X->ex <= -11)
    return 0;
  int sh = 64 - 11 - X->ex;
  int i = X->hi >> sh;
  X->hi = X->hi & ((1ull << sh) - 1);
  normalize (X);
  return i;
}

/* h+l <- c1/2^64 + c0/2^128, rounded towards zero */
static void
set_dd (double *h, double *l, uint64_t c1, uint64_t c0)
{
  uint64_t e, f, g;
  b64u64_u t;
  if (c1)
    {
      e = __builtin_clzll (c1);
      if (e)
        {
          c1 = (c1 << e) | (c0 >> (64 - e));
          c0 = c0 << e;
        }
      f = 0x3fe - e;
      t.u = (f << 52) | ((c1 << 1) >> 12);
      *h = t.f;
      c0 = (c1 << 53) | (c0 >> 11);
      if (c0)
        {
          g = __builtin_clzll (c0);
          if (g)
            c0 = c0 << g;
          t.u = ((f - 53 - g) << 52) | ((c0 << 1) >> 12);
          *l = t.f;
        }
      else
        *l = 0;
    }
  else if (c0)
    {
      e = __builtin_clzll (c0);
      f = 0x3fe - 64 - e;
      c0 = c0 << (e+1); // most significant bit shifted out
      /* put the upper 52 bits of c0 into h */
      t.u = (f << 52) | (c0 >> 12);
      *h = t.f;
      /* put the lower 12 bits of c0 into l */
      c0 = c0 << 52;
      if (c0)
        {
          g = __builtin_clzll (c0);
          c0 = c0 << (g+1);
          t.u = ((f - 64 - g) << 52) | (c0 >> 12);
          *l = t.f;
        }
      else
        *l = 0;
    }
  else
    *h = *l = 0;
  /* Since we truncate from two 64-bit words to a double-double,
     we have another truncation error of less than 2^-106, thus
     the absolute error is bounded as follows:
     | h + l - frac(x/(2pi)) | < 2^-75.999 + 2^-106 < 2^-75.998 */
}

/* Assuming 0x1.7137449123ef6p-26 < x < +Inf,
   return i and set h,l such that i/2^11+h+l approximates frac(x/(2pi)):

   | i/2^11 + h + l - frac(x/(2pi)) | < 2^-104.815,
   with |h| < 2^-11 and |l| < 2^-52.36.
*/
static int
reduce_fast (double *h, double *l, double x)
{
  if (__builtin_expect(x <= 0x1.921fb54442d17p+2, 1)) // x < 2*pi
    {
      /* | CH+CL - 1/(2pi) | < 2^-110.523 */
#define CH 0x1.45f306dc9c883p-3
#define CL -0x1.6b01ec5417056p-57
      a_mul (h, l, CH, x);            // exact
      *l = __builtin_fma (CL, x, *l);
      /* The error in the above fma() is at most ulp(l),
         where |l| <= CL*|x|+|l_in|.
         Let xmax = 0x1.921fb54442d17p+2.
         We have x <= xmax and we can check that CH * xmax < 1.
         Then h <= 1 and |l_in| <= ulp(0.5) = 2^-53,
         where l_in is the value of l after a_mul.
         Then |l| <= |CL|*xmax + 2^-53 < 2^-52.361.
         The rounding error of the fma() is bounded by
         ulp(l) <= ulp(2^-52.361) = 2^-105.
         The error due to the approximation of 1/(2pi)
         is bounded by 2^-110.523*xmax <= 2^-107.871.
         Adding both errors yields:
         |h + l - x/(2pi)| < 2^-105 + 2^-107.871 < 2^-104.815.
      */
    }
  else // x > 0x1.921fb54442d17p+2
    {
      b64u64_u t = {.f = x};
      int e = (t.u >> 52) & 0x7ff; /* 1025 <= e <= 2046 */
      /* We have 2^(e-1023) <= x < 2^(e-1022), thus
         ulp(x) is a multiple of 2^(e-1075), for example
         if x is just above 2*pi, e=1025, 2^2 <= x < 2^3,
         and ulp(x) is a multiple of 2^-50.
         On the other side 1/(2pi) ~ T[0]/2^64 + T[1]/2^128 + T[2]/2^192 + ...
         Let i be the smallest integer such that 2^(e-1075)/2^(64*(i+1))
         is not an integer, i.e., e - 1139 - 64i < 0, i.e.,
         i >= (e-1138)/64. */
      uint64_t m = (1ull << 52) | (t.u & 0xfffffffffffffull);
      uint64_t c[3];
      u128 u, v;
      // x = m/2^53 * 2^(e-1022)
      if (e <= 1074) // 1025 <= e <= 1074: 2^2 <= x < 2^52
        {
          v = u128_mul (u128_from_u64 (m), u128_from_u64 (T[2]));
          u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[1]));
          c[0] = u128_low (u128_add (u, u128_rshift (v, 64)));
          c[1] = u128_high (u) + (c[0] < u128_low (u));
          /* There can be no overflow in (u >> 64) + (c[0] < u) since
             u <= (2^64-1)*T[1] thus (u >> 64) < T[1], and T[1]+1
             does not overflow. */
          u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[0]));
          c[1] += u128_low (u);
          c[2] = u128_high (u) + (c[1] < u128_low (u));
          /* Up to here, we multiplied exactly m by T[0] and T[1] and
             took into account the upper part of m*T[2]:
             c[2]*2^128+c[1]*2^64+c[0] = m*(T[1]+2^64*T[0])+floor(m*T[2]/2^64)
             |c[2]*2^128+c[1]*2^64+c[0]-m/(2pi)*2^128|
                < frac(m*T[2]/2^64) + (T[3]+1)/2^128
                < (2^64-1)/2^64 + (2^64-1)/2^128 < 1
             thus:
             | (c[2]*2^128+c[1]*2^64+c[0])*2^(e-1203) - x/(2pi) | < 2^(e-1203)
             | c[2]*2^(e-1075)+c[1]*2^(e-1139)+c[0]*2^(e-1203) - x/(2pi) |
               < 2^(e-1203)
             The low 1075-e bits of c[2] contribute to frac(x/(2pi)),
             and the ignored part is bounded by 1 with respect to c[0]. */
          e = 1075 - e; // 1 <= e <= 50
          // e is the number of low bits of c[2] contributing to frac(x/(2pi))
        }
      else // 1075 <= e <= 2046, 2^52 <= x < 2^1024
        {
          int i = (e - 1138 + 63) / 64; // i = ceil((e-1138)/64), 0 <= i <= 15
          /* m*T[i] contributes to f = 1139 + 64*i - e bits to frac(x/(2pi))
             with 1 <= f <= 64
             m*T[i+1] contributes a multiple of 2^(-f-64),
                      and at most to 2^(53-f)
             m*T[i+2] contributes a multiple of 2^(-f-128),
                      and at most to 2^(-11-f)
             m*T[i+3] contributes a multiple of 2^(-f-192),
                      and at most to 2^(-75-f) <= 2^-76
          */
          v = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i+3]));
          u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i+2]));
          c[0] = u128_low (u) + u128_low (v);
          /* There can be no overflow in (u >> 64) + (c[0] < u) since
             u <= (2^64-1)*T[i+2] thus (u >> 64) < T[i+2], and T[i+2]+1
             does not overflow. */
          c[1] = u128_high (u) + (c[0] < u128_low (u));
          u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i+1]));
          c[1] += u128_low (u);
          c[2] = u128_high (u) + (c[1] < u128_low (u));
          u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i]));
          c[2] += u128_low (u);
          e = 1139 + (i<<6) - e; // 1 <= e <= 64
          /* Like in the previous case, the ignored part due to the
             ignored low part of m*T[i+3], and to the further terms
             m*T[i+4], ..., is at most 1 relative to c[0]. */
        }
      // e is the number of bits of c[2] to ignore
      if (e == 64)
        {
          /* we ignore all bits of c[0]: the ignored part with respect to
             the new value of c[0] is (2^64-1)/2^64 + 1/2^64 < 1 */
          c[0] = c[1];
          c[1] = c[2];
        }
      else
        {
          /* we ignore the low bits of c[0]: the ignored part with respect to
             the new value of c[0] is (2^64-1)/2^64 + 1/2^64 < 1 */
          c[0] = (c[1] << (64 - e)) | c[0] >> e;
          c[1] = (c[2] << (64 - e)) | c[1] >> e;
        }
      /* In all cases the ignored contribution from low(x*T[2]) and
         x*(T[i+3] + T[i+4] + ...) is less than 1 with respect to c[0],
         thus since we consider (c[1]*2^64+c[0])/2^128, it is < 2^-128. */
      set_dd (h, l, c[1], c[0]);
      /* set_dd() ensures |h| < 1 and |l| < ulp(h) <= 2^-53,
         with truncation error < 2^-106, thus the absolute error is
         bounded by 2^-106 + 2^-128 < 2^-105.999:
         |h + l - fracx/(2pi)| < 2^-105.999. */
    }

  /* In case x < 2pi we have:
     |h + l - x/(2pi)| < 2^-104.815,
  and in case 2pi < x we have:
  |h + l - frac(x/(2pi))| < 2^-105.999,
  thus in all cases:
  |h + l - frac(x/(2pi))| < 2^-104.815.
  
  Since we exclude |h+l| < 2^-37, the induced error
  relative to sin2pi(h+l) is bounded by
  2^-104.815/sin2pi(2^-37) < 2^-70.466. */

  double i = __builtin_floor (*h * 0x1p11);
  *h = __builtin_fma (i, -0x1p-11, *h);
  return i;
}

/* h + l <- (bh + bl) / (ah + al) with |bl|, |al| < 2^-49.47
   with relative error bounded by 2^-96.99:
   | h + l - (bh + bl) / (ah + al) | < 2^-96.99 * |h + l|. */
static inline void
fast_div (double *h, double *l, double bh, double bl, double ah, double al)
{
  /* We use here Karp-Markstein's trick for division:
     let b = bh+bl, a = ah+al, y = o(1/a), and z = o(b*y),
     then the approximation of b/a is z' = z + y*(b-a*z):
     b-a*z' = b-a*(z + y*(b-a*z)) = (b-a*z)*(1-a*y).
     We distinguish two errors:
     * the mathematical error, assuming z + y*(b-a*z) is computed exactly
       (but taking into account that y is not exactly 1/a, and z is not
       exactly b/a
     * the rounding errors in z + y*(b-a*z)
     For the error analysis, we assume 1 <= ah, bh < 2 for now.
  */
     
  double y = 1.0 / ah;
  /* y = 1/ah / (1 + eps1) with |eps1| < 2^-52.
     |1-ah*y| < |eps1| < 2^-52. */
  *h = bh * y;
  /* h = bh * y / (1 + eps2) with |eps2| < 2^-52
       = bh/ah / (1 + eps1) / (1 + eps2)
     thus writing z = h:
     bh = ah*z * (1 + eps1) * (1 + eps2)
     |bh - ah*z| < ah*z * (eps1 + eps2 + eps1*eps2)
     Since ah < 2 and z <= bh*y < 2, we have:
     |bh - ah*z| < 4 * (2*2^-52 + 2^-104) < 2^-48.999.
     It follows |bh-ah*z'| < (bh-ah*z)*(1-ah*y) < 2^-48.999*2^-52 < 2^-100.999
     Dividing by ah>=1 yields: |bh/ah-z'| < 2^-100.999 too.
     We assume the same bound hold for b,a: |b/a-z'| < 2^-100.999.
  */

  double eh = __builtin_fma (ah, -*h, bh);
  /* from the analysis above, we have |eh| < 2^-48.999 thus the rounding error
     is bounded by ulp(2^-48.999) = 2^-101 */
  double el = __builtin_fma (al, -*h, bl);
  /* here |al|, |bl| < 2^-49.47 and |h| < 2, thus |el| < 3*2^-49.47 and
     the rounding error is bounded by ulp(3*2^-49.47) = 2^-100. */
  *l = y * (eh + el);
  /* we have |eh+el| < 2^-48.999+3*2^-49.47 < 2^-47.33 thus the rounding error
     on eh+el is bounded by ulp(2^-47.33) = 2^-100.
     Then since y <= 1, the rounding error on l is bounded by 2^-100 too.
     The total rounding error is thus bounded by:
     2^-101+3*2^-100 < 2^-98.19.
     Adding the mathematical error yields:
     2^-100.999 + 2^-98.19 < 2^-97.99:
     | h + l - (bh + bl) / (ah + al) | < 2^-97.99.

     This was assuming 1 <= a,b < 2, thus with 1/2 <= h+l <= 2.
     For the relative error, this corresponds to 2^-97.99/(1/2) = 2^-96.99
     where 1/2 is the smallest possible value of h+l. */
}

/* given a finite input x with |x| > 0x1.d12ed0af1a27ep-27,
   put in h + l an approximation of tan(x),
   return the maximal absolute error err such that
   | h + l - tan(x) | < err */
static double
tan_fast (double *h, double *l, double x)
{
  int neg = x < 0, is_tan = 1;
  double absx = neg ? -x : x;

  /* now absx > 0x1.d12ed0af1a27ep-27 */
  int i = reduce_fast (h, l, absx);
  /* | i/2^11 + h + l - frac(x/(2pi)) | < 2^-104.815 */

  // if i >= 2^10: 1/2 <= frac(x/(2pi)) < 1 thus pi <= x <= 2pi
  // we use tan(pi+x) = tan(x)
  i = i & 0x3ff;
  // | i/2^11 + h + l - frac(x/(2pi)) | mod 1/2 < 2^-104.815

  // now i < 2^10
  // if i >= 2^9: 1/4 <= frac(x/(2pi)) < 1/2 thus pi/2 <= x <= pi
  // we use tan(pi/2+x) = -cot(x)
  is_tan = is_tan ^ (i >> 9);
  neg = neg ^ (i >> 9);
  i = i & 0x1ff;
  // | i/2^11 + h + l - frac(x/(2pi)) | mod 1/4 < 2^-104.815

  // now 0 <= i < 2^9
  // if i >= 2^8: 1/8 <= frac(x/(2pi)) < 1/4
  // we use tan(pi/2-x) = cot(x)
  if (i & 0x100) // case pi/4 <= x_red <= pi/2
    {
      is_tan = !is_tan;
      i = 0x1ff - i;
      /* 0x1p-11 - h is exact below: indeed, reduce_fast first computes
         a first value of h (say h0, with 0 <= h0 < 1), then i = floor(h0*2^11)
         and h1 = h0 - 2^11*i with 0 <= h1 < 2^-11.
         If i >= 2^8 here, this implies h0 >= 1/2^3, thus ulp(h0) >= 2^-55:
         h0 and h1 are integer multiples of 2^-55.
         Thus h1 = k*2^-55 with 0 <= k < 2^44 (since 0 <= h1 < 2^-11).
         Then 0x1p-11 - h = (2^44-k)*2^-55 is exactly representable.
         We can have a huge cancellation in 0x1p-11 - h, for example for
         x = 0x1.61a3db8c8d129p+1023 where we have before this operation
         h = 0x1.ffffffffff8p-12, and h = 0x1p-53 afterwards. */
      *h = 0x1p-11 - *h;
      *l = -*l;
    }

  /* The analysis of reduce_fast() proves that if we except i=0 and h<2^-37,
     then the error of reduce_fast() relative to both sin2pi(R) and cos2pi(R)
     is bounded by 2^-70.466:
     | R - frac(x/(2pi)) mod 1/4 | < 2^-70.466 * |sin2pi(R)|
     | R - frac(x/(2pi)) mod 1/4 | < 2^-70.466 * |cos2pi(R)|
     where R = i/2^11 + h + l, thus since the derivative of sin2pi and
     cos2pi is bounded by 2*pi and 2*pi*2^-70.466 < 2^-67.814:
     | sin2pi(R) - sin(x mod pi/2) | < 2^-67.814 * |sin2pi(R)|.
     | cos2pi(R) - cos(x mod pi/2) | < 2^-67.814 * |cos2pi(R)|.
     For i=0 and h<2^-37, we defer to the slow path. */
  if (__builtin_expect (i==0 && *h < 0x1p-37, 0))
    return 0x1p0;

  /* Now 0 <= i < 256 and 0 <= h+l < 2^-11
     with | i/2^11 + h + l - frac(x/(2pi)) | cmod 1/4 < 2^-104.815
     If is_tan=1, tan |x| = tan2pi (R + err1) with |err1| < 2^-104.815,
     if is_tan=0, tan |x| = cot2pi (R + err1) with |err1| < 2^-104.815.
     In both cases R = i/2^11 + h + l, 0 <= R < 1/4.
  */
  double sh, sl, ch, cl;
  /* since the SC[] table evaluates at i/2^11 + SC[i][0] and not at i/2^11,
     we must subtract SC[i][0] from h+l */
  /* Here h = k*2^-55 with 0 <= k < 2^44, and SC[i][0] is an integer
     multiple of 2^-62, with |SC[i][0]| < 2^-24, thus SC[i][0] = m*2^-62
     with |m| < 2^38. It follows h-SC[i][0] = (k*2^7 + m)*2^-62 with
     2^51 - 2^38 < k*2^7 + m < 2^51 + 2^38, thus h-SC[i][0] is exact. */
  *h -= SC[i][0];
  /* the following fast_two_sum() guarantees that |l| <= ulp(h) thus
     |l| <= 2^-52 |h| at input of evalPSfast() and evalPCfast() */
  fast_two_sum (h, l, *h, *l);
  // now -2^-24 < h < 2^-11+2^-24
  // from comments in reduce_fast() we have |l| < 2^-52.36
  double uh, ul;
  a_mul (&uh, &ul, *h, *h);
  ul = __builtin_fma (*h + *h, *l, ul);
  // uh+ul approximates (h+l)^2
  evalPSfast (&sh, &sl, *h, *l, uh, ul);
  /* the relative error of evalPSfast() is less than 2^-71.61 from
     routine evalPSfast_all(K=8) in tan.sage:
     | sh + sh - sin2pi(h+l) | < 2^-71.61 * |sin2pi(h+l)| */
  evalPCfast (&ch, &cl, uh, ul);
  /* the relative error of evalPCfast() is less than 2^-69.96 from
     routine evalPCfast() in sin.sage:
     | ch + cl - cos2pi(h+l) | < 2^-69.96 * |cos2pi(h+l)| */

  double sh0, sl0, ch0, cl0, h1, l1;
  s_mul (&sh0, &sl0, SC[i][2], sh, sl);
  s_mul (&ch0, &cl0, SC[i][1], ch, cl);
  fast_two_sum (&h1, &l1, ch0, sh0);
  l1 += sl0 + cl0;
  /* relative error bounded by 2^-67.777
     from global_error(is_sin=true,rel=true) in tan.sage:
     | h1 + l1 - sin2pi (R) | < 2^-67.777 * |sin2pi(R)|
     with in addition |l1| < 2^-49.47 */

  double h2, l2;
  s_mul (&ch, &cl, SC[i][2], ch, cl);
  s_mul (&sh, &sl, SC[i][1], sh, sl);
  fast_two_sum (&h2, &l2, ch, -sh);
  l2 += cl - sl;
  /* relative error bounded by 2^-68.073
     from global_error(is_sin=false,rel=true) in sin.sage:
     | h2 + l2 - cos2pi (R) | < 2^-68.073 * |cos2pi(R)|
     (where cos|x| has to be replaced by sin|x| for is_tan=0)
     with in addition |l2| < 2^-49.62 */

  /* here we have |l1|, |l1| < 2^-49.47 */
  if (is_tan)
    fast_div (h, l, h1, l1, h2, l2);
    /* |h_out+l_out - (h_in+l_in)/(hh+ll)| < 2^-96.99 * |h_out+l_out| */
  else
    fast_div (h, l, h2, l2, h1, l1);

  /* In summary we have when is_tan=1:
     h1+l1 = sin2pi(R) * (1 + eps1) with |eps1| < 2^-67.777
     h2+l2 = cos2pi(R) * (1 + eps2) with |eps2| < 2^-68.073
     sin2pi(R) = sin(x mod pi/2) * (1 + eps3) with |eps3| < 2^-67.814
     cos2pi(R) = cos(x mod pi/2) * (1 + eps4) with |eps4| < 2^-67.814
     h+l = (h1+l1)/(h2+l2) * (1 + eps5) with |eps5| < 2^-96.99
     This yields:
     h+l = tan(x mod pi/2) * (1+eps1)*(1+eps3)*(1+eps5)/(1+eps2)/(1+eps4)
     The largest value is obtained when eps1,eps3,eps5 are maximum, and
     eps2,eps4 minimum, and we get:
     h+l = tan(x mod pi/2) * (1+eps) with |eps| < 2^-65.864
     (the same bound holds for eps1,eps3,eps5 minimum and eps2,eps4 maximum).
     When is_tan=0, we get the same reasoning with inverse ratio, but the
     bounds are the same.
  */

  static const double sgn[2] = {1.0, -1.0};
  *h *= sgn[neg];
  *l *= sgn[neg];
  return *h * 0x1.1ap-66; // 2^-65.864 < 0x1.1ap-66
}

/* Assume x is a regular number, and |x| > 0x1.d12ed0af1a27ep-27. */
static double
tan_accurate (double x)
{
  double absx = (x > 0) ? x : -x;

  dint64_t X[1];
  dint_fromd (X, absx);

  /* reduce argument */
  reduce (X);
  
  // now |X - x/(2pi) mod 1| < 2^-126.67*X, with 0 <= X < 1.

  int is_tan = 1, neg = x < 0;

  // Write X = i/2^11 + r with 0 <= r < 2^11.
  int i = reduce2 (X); // exact

  i = i & 0x3ff; // if pi <= x < 2*pi, tan(x) = tan(x-pi)

  // now i < 2^10

  if (i & 0x200) // pi/2 <= x < pi: tan(x) = -1/tan(x-pi/2)
  {
    is_tan = 0;
    neg = !neg;
    i = i & 0x1ff;
  }

  // now 0 <= i < 2^9

  if (i & 0x100)
    // pi/4 <= x < pi/2: tan(x) = 1/tan(pi/2-x)
  {
    is_tan = !is_tan;
    X->sgn = 1; // negate X
    add_dint (X, &MAGIC, X); // X -> 2^-11 - X
    // here: 256 <= i <= 511
    i = 0x1ff - i;
    // now 0 <= i < 256
  }

  // now 0 <= i < 256 and 0 <= X < 2^-11

  /* If is_tan=1, tan |x| = tan2pi (R * (1 + eps))
        (cases 0 <= x < pi/4 and 3pi/4 <= x < pi)
     if is_tan=0, tan |x| = cot2pi (R * (1 + eps))
        (case pi/4 <= x < 3pi/4)
     In both cases R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
  */

  dint64_t U[1], V[1], X2[1];
  mul_dint (X2, X, X);       // X2 approximates X^2
  evalPC (U, X2);    // cos2pi(X)
  /* since 0 <= X < 2^-11, we have 0.999 < U <= 1 */
  evalPS (V, X, X2); // sin2pi(X)
  /* since 0 <= X < 2^-11, we have 0 <= V < 0.0005 */

  // sin2pi(R) ~ sin2pi(i/2^11)*cos2pi(X)+cos2pi(i/2^11)*sin2pi(X)
  dint64_t Sin[1], UU[1], VV[1];
  mul_dint (UU, S+i, U);
  /* since 0 <= S[i] < 0.705 and 0.999 < Uin <= 1, we have
     0 <= U < 0.705 */
  mul_dint (VV, C+i, V);
  add_dint (Sin, UU, VV);
  /* For the error analysis, we distinguish the case i=0.
     For i=0, we have S[i]=0 and C[1]=1, thus V is the value computed
     by evalPS() above, with relative error < 2^-124.648.
     
     For 1 <= i < 256, analyze_sin_case1(rel=true) from sin.sage gives a
     relative error bound of -122.797 (obtained for i=1).
     In all cases, the relative error for the computation of
     sin2pi(i/2^11)*cos2pi(X)+cos2pi(i/2^11)*sin2pi(X) is bounded by -122.797
     not taking into account the approximation error in R:
     |S - sin2pi(R)| < |S| * 2^-122.797.

     For the approximation error in R, we have:
     sin |x| = sin2pi (R * (1 + eps))
     R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
     Thus sin|x| = sin2pi(R+R*eps)
     = sin2pi(R)+R*eps*2*pi*cos2pi(theta), theta in [R,R+R*eps]
     Since 2*pi*R/sin(2*pi*R) < pi/2 for R < 1/4, it follows:
     | sin|x| - sin2pi(R) | < pi/2*R*|sin(2*pi*R)|
     | sin|x| - sin2pi(R) | < 2^-126.018 * |sin2pi(R)|.

     Adding both errors we get:
     | sin|x| - S | < |S| * 2^-122.797 + 2^-126.018 * |sin2pi(R)|
     < |S| * 2^-122.797 + 2^-126.018 * |S| * (1 + 2^-122.797)
     < |S| * 2^-122.650.
  */
  
  // cos2pi(R) ~ cos2pi(i/2^11)*cos2pi(X)-sin2pi(i/2^11)*sin2pi(X)
  dint64_t Cos[1];
  mul_dint (U, C+i, U);
  mul_dint (V, S+i, V);
  V->sgn = 1 - V->sgn; // negate V
  add_dint (Cos, U, V);
  /* For 0 <= i < 256, analyze_sin_case2(rel=true) from sin.sage gives a
     relative error bound of -123.540 (obtained for i=0):
     |C - cos2pi(R)| < |C| * 2^-123.540.

     For the approximation error in R, we have:
     sin |x| = cos2pi (R * (1 + eps))
     R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
     Thus sin|x| = cos2pi(R+R*eps)
                 = cos2pi(R)-R*eps*2*pi*sin2pi(theta), theta in [R,R+R*eps]
     Since we have R < 1/4, we have cos2pi(R) >= sqrt(2)/2,
     and it follows:
     | sin|x|/cos2pi(R) - 1 | < 2*pi*R*eps/(sqrt(2)/2)
                              < pi/2*eps/sqrt(2)          [since R < 1/4]
                              < 2^-126.518.
     Adding both errors we get:
     | cos|x| - C | < |C| * 2^-123.540 + 2^-126.518 * |cos2pi(R)|
                    < |C| * 2^-123.540 + 2^-126.518 * |C| * (1 + 2^-123.540)
                    < |C| * 2^-123.367.
  */

  /* Now if is_tan=1 we compute S/C, otherwise we compute C/S. */
  if (is_tan)
    div_dint (U, Sin, Cos);
  else
    div_dint (U, Cos, Sin);

  /* Relative error on S < eps1 = 2^-122.650
     Relative error on C < eps2 = 2^-123.367
     Relative error of div_dint() < eps3 = 2^-123.67
     Total relative error < (1+eps1)*(1+eps2)*(1+eps3)-1 < 2^-121.578,
     this is less than 2^-121.578/2^-128 < 86 ulps. */

  uint64_t err = 86;
  uint64_t hi0, hi1, lo0, lo1;
  lo0 = U->lo - err;
  hi0 = U->hi - (lo0 > U->lo);
  lo1 = U->lo + err;
  hi1 = U->hi + (lo1 < U->lo);
  /* check the upper 54 bits are equal */
  if ((hi0 >> 10) != (hi1 >> 10))
    {
      static const double exceptions[2][3] = {
        /* the following has 78 identical bits after the round bit */
        {0x1.dffffffffff1fp-22, 0x1.e000000000151p-22, 0x1.fffffffffffffp-76},
        /* the following has 72 identical bits after the round bit */
        {0x1.dfffffffffc7cp-21, 0x1.e000000000546p-21, -0x1.658bcedb6e1d4p-147},
      };
      for (int j = 0; j < 2; j++)
        {
          if (__builtin_fabs (x) == exceptions[j][0])
            return (x > 0) ? exceptions[j][1] + exceptions[j][2]
              : -exceptions[j][1] - exceptions[j][2];
        }
      __builtin_unreachable ();
    }

  if (neg)
    U->sgn = 1 - U->sgn;

  double y = dint_tod (U);

  return y;
}

double
SECTION
__tan (double x)
{
  b64u64_u t = {.f = x};
  int e = (t.u >> 52) & 0x7ff;

  if (__builtin_expect (e == 0x7ff, 0)) /* NaN, +Inf and -Inf. */
  {
#ifdef CORE_MATH_SUPPORT_ERRNO
    if ((t.u << 1) == 0x7ffull<<53) // Inf
      errno = EDOM;
#endif
    if ((t.u << 1) != 0x7ff8ull<<49){
      return 0.0 / 0.0;
    }
    t.u = ~0ull;
    return t.f;
  }
  
  /* now x is a regular number */

  /* For |x| <= 0x1.d12ed0af1a27ep-27, tan(x) rounds to x (to nearest):
     we can assume x >= 0 without loss of generality since tan(-x) = -tan(x),
     we have x < tan(x) < x + x^3/3 for say 0 < x <= 1 thus
     |tan(x) - x| < x^3/3.
     Write x = c*2^e with 1/2 <= c < 1.
     Then ulp(x)/2 = 2^(e-54), and x^3/3 = c^3/3*2^(3e), thus
     x^3/3 < ulp(x)/2 rewrites as c^3/3*2^(3e) < 2^(e-54),
     or c^3*2^(2e+54) < 3 (1).
     For e <= -27, since c^3 < 1, we have c^3*2^(2e+54) < 1 < 3.
     For e=-26, (1) rewrites 4*c^3 < 3 which yields c <= 0x1.d12ed0af1a27ep-1.
  */
  uint64_t ux = t.u & 0x7fffffffffffffff;
  if (ux <= 0x3e4d12ed0af1a27eull) { // |x| <= 0x1.d12ed0af1a27ep-27
    if (x == 0)
      return x;
    // Taylor expansion of tan(x) is x + x^3/3 around zero
      /* We have underflow exactly when 0 < |x| < 2^-1022:
         for RNDU, tan(2^-1022-2^-1074) would round to 2^-1022-2^-1075
         with unbounded exponent range */
#ifdef CORE_MATH_SUPPORT_ERRNO
    if (x != 0 && __builtin_fabs (x) < 0x1p-1022)
      errno = ERANGE; // underflow
#endif
    return __builtin_fma (x, 0x1p-54, x);
  }

  double h, l, err;
  err = tan_fast (&h, &l, x);
  double left  = h + (l - err), right = h + (l + err);
  if (__builtin_expect (left == right, 1))
    return left;

  return tan_accurate (x);
}

#ifndef __tan
libm_alias_double (__tan, tan)
#endif
