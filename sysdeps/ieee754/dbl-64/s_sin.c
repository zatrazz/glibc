/* Correctly-rounded sine function for binary64 value.

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
#include <errno.h>
#include <get-rounding-mode.h>
#include <libm-alias-double.h>
#define CORE_MATH_SUPPORT_ERRNO
#include "dint.h"
#include "s_sincos_data.h"

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
   for 2^-24 <= xh+xl < 2^-11 + 2^-24,
   and |xl| < 2^-52.36, with absolute error < 2^-77.09
   (see evalPSfast() in sin.sage).
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
   for 2^-24 <= xh+xl < 2^-11 + 2^-24,
   and |xl| < 2^-52.36, with relative error < 2^-69.96
   (see evalPCfast() in sin.sage).
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

/* h+l <- c1/2^64 + c0/2^128 */
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
   return i and set h,l such that i/2^11+h+l approximates frac(x/(2pi)).
   If x <= 0x1.921fb54442d18p+2:
   | i/2^11 + h + l - frac(x/(2pi)) | < 2^-104.116 * |i/2^11 + h + l|
   with |h| < 2^-11 and |l| < 2^-52.36.

   Otherwise only the absolute error is bounded:
   | i/2^11 + h + l - frac(x/(2pi)) | < 2^-75.998
   with 0 <= h < 2^-11 and |l| < 2^-53.

   In both cases we have |l| < 2^-51.64*|i/2^11 + h|.

   Put in err1 a bound for the absolute error:
   | i/2^11 + h + l - frac(x/(2pi)) |.
*/
static int
reduce_fast (double *h, double *l, double x, double *err1)
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
         Assume 2^(e-1) <= x < 2^e.
         Then |h| < 2^(e-2) and |l_in| <= 1/2 ulp(2^(e-2)) = 2^(e-55),
         where l_in is the value of l after a_mul.
         Then |l| <= CL*x + 2^(e-55) <= 2^e*(CL+2-55) < 2^e * 2^-55.6.
         The rounding error of the fma() is bounded by
         ulp(l) <= 2^e * ulp(2^-55.6) = 2^(e-108).
         The error due to the approximation of 1/(2pi)
         is bounded by 2^-110.523*x <= 2^(e-110.523).
         Adding both errors yields:
         |h + l - x/(2pi)| < 2^e * (2^-108 + 2^-110.523) < 2^e * 2^-107.768.
         Since |x/(2pi)| > 2^(e-1)/(2pi), the relative error is bounded by:
         2^e * 2^-107.768 / (2^(e-1)/(2pi)) = 4pi * 2^-107.768 < 2^-104.116.

         Bound on l: since |h| < 1, we have after |l| <= ulp(h) <= 2^-53
         after a_mul(), and then |l| <= |CL|*0x1.921fb54442d17p+2 + 2^-53
         < 2^-52.36.

         Bound on l relative to h: after a_mul() we have |l| <= ulp(h)
         <= 2^-52*h. After fma() we have |l| <= CL*x + 2^-52*h
         <= 2^-53.84*CH*x + 2^-52*h <= (2^-53.84+2^-52)*h < 2^-51.64*h.
      */
      *err1 = 0x1.d9p-105 * *h; // error < 2^-104.116 * h
    }
  else // x > 0x1.921fb54442d17p+2
    {
      b64u64_u t = {.f = x};
      int e = (t.u >> 52) & 0x7ff; /* 1025 <= e <= 2046 */
      /* We have 2^(e-1023) <= x < 2^(e-1022), thus
         ulp(x) is a multiple of 2^(e-1075), for example
         if x is just above 2*pi, e=1025, 2^2 <= x < 2^e,
         and ulp(x) is a multiple of 2^-50.
         On the other side 1/(2pi) ~ T[0]/2^64 + T[1]/2^128 + T[2]/2^192 + ...
         Let i be the smallest integer such that 2^(e-1075)/2^(64*(i+1))
         is not an integer, i.e., e - 1139 - 64i < 0, i.e.,
         i >= (e-1138)/64. */
      uint64_t m = (1ull << 52) | (t.u & 0xfffffffffffffull);
      uint64_t c[3];
      u128 u;
      // x = m/2^53 * 2^(e-1022)
      if (e <= 1074) // 1025 <= e <= 1074: 2^2 <= x < 2^52
        {
          /* In that case the contribution of x*T[2]/2^192 is less than
             2^(52+64-192) <= 2^-76. */
	  u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[1]));
          c[0] = u128_low (u);
	  c[1] = u128_high (u);
	  u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[0]));
	  c[1] += u128_low (u);
          c[2] = u128_high (u) + (c[1] < u128_low (u));
          /* | c[2]*2^128+c[1]*2^64+c[0] - m/(2pi)*2^128 | < m*T[2]/2^64 < 2^53
             thus:
             | (c[2]*2^128+c[1]*2^64+c[0])*2^(e-1203) - x/(2pi) | < 2^(e-1150)
             The low 1075-e bits of c[2] contribute to frac(x/(2pi)).
          */
          e = 1075 - e; // 1 <= e <= 50
          // e is the number of low bits of C[2] contributing to frac(x/(2pi))
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
	  u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i+2]));
	  c[0] = u128_low (u);
	  c[1] = u128_high (u);
	  u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i+1]));
	  c[1] += u128_low (u);
          c[2] = u128_high (u) + (c[1] < u128_low (u));
          u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i]));
	  c[2] += u128_low (u);
          e = 1139 + (i<<6) - e; // 1 <= e <= 64
          // e is the number of low bits of C[2] contributing to frac(x/(2pi))
        }
      if (e == 64)
        {
          c[0] = c[1];
          c[1] = c[2];
        }
      else
        {
          c[0] = (c[1] << (64 - e)) | c[0] >> e;
          c[1] = (c[2] << (64 - e)) | c[1] >> e;
        }
      /* In all cases the ignored contribution from x*T[2] or x*T[i+3]
         is less than 2^-76,
         and the truncated part from the above shift is less than 2^-128 thus:
         | c[1]/2^64 + c[0]/2^128 - frac(x/(2pi)) | < 2^-76+2^-128 < 2^-75.999
      */
      set_dd (h, l, c[1], c[0]);
      /* set_dd() ensures |h| < 1 and |l| < ulp(h) <= 2^-53 */
      *err1 = 0x1.01p-76;
    }

  double i = __builtin_floor (*h * 0x1p11);
  *h = __builtin_fma (i, -0x1p-11, *h);
  return i;
}

/* return the maximal absolute error */
static double
sin_fast (double *h, double *l, double x)
{
  int neg = x < 0, is_sin = 1;
  double absx = neg ? -x : x;

  /* now x > 0x1.7137449123ef6p-26 */
  double err1;
  int i = reduce_fast (h, l, absx, &err1);
  /* err1 is an absolute bound for | i/2^11 + h + l - frac(x/(2pi)) |:
     | i/2^11 + h + l - frac(x/(2pi)) | < err1 */

  // if i >= 2^10: 1/2 <= frac(x/(2pi)) < 1 thus pi <= x <= 2pi
  // we use sin(pi+x) = -sin(x)
  neg = neg ^ (i >> 10);
  i = i & 0x3ff;
  // | i/2^11 + h + l - frac(x/(2pi)) | mod 1/2 < err1

  // now i < 2^10
  // if i >= 2^9: 1/4 <= frac(x/(2pi)) < 1/2 thus pi/2 <= x <= pi
  // we use sin(pi/2+x) = cos(x)
  is_sin = is_sin ^ (i >> 9);
  i = i & 0x1ff;
  // | i/2^11 + h + l - frac(x/(2pi)) | mod 1/4 < err1

  // now 0 <= i < 2^9
  // if i >= 2^8: 1/8 <= frac(x/(2pi)) < 1/4
  // we use sin(pi/2-x) = cos(x)
  if (i & 0x100) // case pi/4 <= x_red <= pi/2
    {
      is_sin = !is_sin;
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
         h = 0x1.ffffffffff8p-12, and h = 0x1p-53 afterwards. But this
         does not hurt since we bound the absolute error and not the
         relative error at the end. */
      *h = 0x1p-11 - *h;
      *l = -*l;
    }

  /* Now 0 <= i < 256 and 0 <= h+l < 2^-11
     with | i/2^11 + h + l - frac(x/(2pi)) | cmod 1/4 < err1
     If is_sin=1, sin |x| = sin2pi (R + err1);
     if is_sin=0, sin |x| = cos2pi (R + err1).
     In both cases R = i/2^11 + h + l, 0 <= R < 1/4.
  */
  double sh, sl, ch, cl;
  /* since the SC[] table evaluates at i/2^11 + SC[i][0] and not at i/2^11,
     we must subtract SC[i][0] from h+l */
  /* Here h = k*2^-55 with 0 <= k < 2^44, and SC[i][0] is an integer
     multiple of 2^-62, with |SC[i][0]| < 2^-24, thus SC[i][0] = m*2^-62
     with |m| < 2^38. It follows h-SC[i][0] = (k*2^7 + m)*2^-62 with
     2^51 - 2^38 < k*2^7 + m < 2^51 + 2^38, thus h-SC[i][0] is exact.
     Now |h| < 2^-11 + 2^-24. */
  *h -= SC[i][0];
  // now -2^-24 < h < 2^-11+2^-24
  // from reduce_fast() we have |l| < 2^-52.36
  double uh, ul;
  a_mul (&uh, &ul, *h, *h);
  ul = __builtin_fma (*h + *h, *l, ul);
  // uh+ul approximates (h+l)^2
  evalPSfast (&sh, &sl, *h, *l, uh, ul);
  /* the absolute error of evalPSfast() is less than 2^-77.09 from
     routine evalPSfast() in sin.sage:
     | sh + sh - sin2pi(h+l) | < 2^-77.09 */
  evalPCfast (&ch, &cl, uh, ul);
  /* the relative error of evalPCfast() is less than 2^-69.96 from
     routine evalPCfast(rel=true) in sin.sage:
     | ch + cl - cos2pi(h+l) | < 2^-69.96 * |ch + cl| */
  double err;
  static const double sgn[2] = {1.0, -1.0};
  if (is_sin)
    {
      s_mul (&sh, &sl, sgn[neg] * SC[i][2], sh, sl);
      s_mul (&ch, &cl, sgn[neg] * SC[i][1], ch, cl);
      fast_two_sum (h, l, ch, sh);
      *l += sl + cl;
      /* absolute error bounded by 2^-68.588
         from global_error(is_sin=true,rel=false) in sin.sage:
         | h + l - sin2pi (R) | < 2^-68.588
         thus:
         | h + l - sin |x| | < 2^-68.588 + | sin2pi (R) - sin |x| |
                             < 2^-68.588 + err1 */
      err = 0x1.55p-69; // 2^-66.588 < 0x1.55p-69
    }
  else
    {
      s_mul (&ch, &cl, sgn[neg] * SC[i][2], ch, cl);
      s_mul (&sh, &sl, sgn[neg] * SC[i][1], sh, sl);
      fast_two_sum (h, l, ch, -sh);
      *l += cl - sl;
      /* absolute error bounded by 2^-68.414
         from global_error(is_sin=false,rel=false) in sin.sage:
         | h + l - cos2pi (R) | < 2^-68.414
         thus:
         | h + l - sin |x| | < 2^-68.414 + | cos2pi (R) - sin |x| |
                             < 2^-68.414 + err1 */
      err = 0x1.81p-69; // 2^-68.414 < 0x1.81p-69
    }
  // *h *= sgn[neg];
  // *l *= sgn[neg];
  return err + err1;
}

/* Assume x is a regular number, and |x| > 0x1.7137449123ef6p-26. */
__attribute__((cold))
static double
sin_accurate (double x)
{
  double absx = (x > 0) ? x : -x;

  dint64_t X[1];
  dint_fromd (X, absx);

  /* reduce argument */
  reduce (X);
  
  // now |X - x/(2pi) mod 1| < 2^-126.67*X, with 0 <= X < 1.

  int neg = x < 0, is_sin = 1;

  // Write X = i/2^11 + r with 0 <= r < 2^11.
  int i = reduce2 (X); // exact

  if (i & 0x400) // pi <= x < 2*pi: sin(x) = -sin(x-pi)
  {
    neg = !neg;
    i = i & 0x3ff;
  }

  // now i < 2^10

  if (i & 0x200) // pi/2 <= x < pi: sin(x) = cos(x-pi/2)
  {
    is_sin = 0;
    i = i & 0x1ff;
  }

  // now 0 <= i < 2^9

  if (i & 0x100)
    // pi/4 <= x < pi/2: sin(x) = cos(pi/2-x), cos(x) = sin(pi/2-x)
  {
    is_sin = !is_sin;
    X->sgn = 1; // negate X
    add_dint (X, &MAGIC, X); // X -> 2^-11 - X
    // here: 256 <= i <= 511
    i = 0x1ff - i;
    // now 0 <= i < 256
  }

  // now 0 <= i < 256 and 0 <= X < 2^-11

  /* If is_sin=1, sin |x| = sin2pi (R * (1 + eps))
        (cases 0 <= x < pi/4 and 3pi/4 <= x < pi)
     if is_sin=0, sin |x| = cos2pi (R * (1 + eps))
        (case pi/4 <= x < 3pi/4)
     In both cases R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
  */

  dint64_t U[1], V[1], X2[1];
  mul_dint (X2, X, X);       // X2 approximates X^2
  evalPC (U, X2);    // cos2pi(X)
  /* since 0 <= X < 2^-11, we have 0.999 < U <= 1 */
  evalPS (V, X, X2); // sin2pi(X)
  /* since 0 <= X < 2^-11, we have 0 <= V < 0.0005 */
  if (is_sin)
  {
    // sin2pi(R) ~ sin2pi(i/2^11)*cos2pi(X)+cos2pi(i/2^11)*sin2pi(X)
    mul_dint (U, S+i, U);
    /* since 0 <= S[i] < 0.705 and 0.999 < Uin <= 1, we have
       0 <= U < 0.705 */
    mul_dint (V, C+i, V);
    /* For the error analysis, we distinguish the case i=0.
       For i=0, we have S[i]=0 and C[1]=1, thus V is the value computed
       by evalPS() above, with relative error < 2^-124.648.

       For 1 <= i < 256, analyze_sin_case1(rel=true) from sin.sage gives a
       relative error bound of -122.797 (obtained for i=1).
       In all cases, the relative error for the computation of
       sin2pi(i/2^11)*cos2pi(X)+cos2pi(i/2^11)*sin2pi(X) is bounded by -122.797
       not taking into account the approximation error in R:
       |U - sin2pi(R)| < |U| * 2^-122.797, with U the value computed
       after add_dint (U, U, V) below.

       For the approximation error in R, we have:
       sin |x| = sin2pi (R * (1 + eps))
       R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
       Thus sin|x| = sin2pi(R+R*eps)
                   = sin2pi(R)+R*eps*2*pi*cos2pi(theta), theta in [R,R+R*eps]
       Since 2*pi*R/sin(2*pi*R) < pi/2 for R < 1/4, it follows:
       | sin|x| - sin2pi(R) | < pi/2*R*|sin(2*pi*R)|
       | sin|x| - sin2pi(R) | < 2^-126.018 * |sin2pi(R)|.

       Adding both errors we get:
       | sin|x| - U | < |U| * 2^-122.797 + 2^-126.018 * |sin2pi(R)|
                      < |U| * 2^-122.797 + 2^-126.018 * |U| * (1 + 2^-122.797)
                      < |U| * 2^-122.650.
    */
  }
  else
  {
    // cos2pi(R) ~ cos2pi(i/2^11)*cos2pi(X)-sin2pi(i/2^11)*sin2pi(X)
    mul_dint (U, C+i, U);
    mul_dint (V, S+i, V);
    V->sgn = 1 - V->sgn; // negate V
    /* For 0 <= i < 256, analyze_sin_case2(rel=true) from sin.sage gives a
       relative error bound of -123.540 (obtained for i=0):
       |U - cos2pi(R)| < |U| * 2^-123.540, with U the value computed
       after add_dint (U, U, V) below.

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
       | sin|x| - U | < |U| * 2^-123.540 + 2^-126.518 * |cos2pi(R)|
                      < |U| * 2^-123.540 + 2^-126.518 * |U| * (1 + 2^-123.540)
                      < |U| * 2^-123.367.
    */
  }
  add_dint (U, U, V);
  /* If is_sin=1:
     | sin|x| - U | < |U| * 2^-122.650
     If is_sin=0:
     | cos|x| - U | < |U| * 2^-123.367.
     In all cases the total error is bounded by |U| * 2^-122.650.
     The term |U| * 2^-122.650 contributes to at most 2^(128-122.650) < 41 ulps
     relatively to U->lo.
  */
  uint64_t err = 41;
  uint64_t hi0, hi1, lo0, lo1;
  lo0 = U->lo - err;
  hi0 = U->hi - (lo0 > U->lo);
  lo1 = U->lo + err;
  hi1 = U->hi + (lo1 < U->lo);
  /* check the upper 54 bits are equal */
  if ((hi0 >> 10) != (hi1 >> 10))
    {
      static const double exceptions[][3] = {
        {0x1.e0000000001c2p-20, 0x1.dfffffffff02ep-20, 0x1.dcba692492527p-146},
        /* the following worst case was reported by Erik E., it has 68
           identical bits after the round bit */
        {0x1.6ac5b262ca1ffp+849, 0x1p+0, -0x1.2b089ea1e692bp-123},
      };
      for (int j = 0; j < 2; j++)
        {
          if (__builtin_fabs (x) == exceptions[j][0])
            return (x > 0) ? exceptions[j][1] + exceptions[j][2]
              : -exceptions[j][1] - exceptions[j][2];
        }
#if 0
      printf ("Rounding test of accurate path failed for sin(%la)\n", x);
      printf ("Please report the above to core-math@inria.fr\n");
      exit (1);
#else
      __builtin_unreachable ();
#endif
    }

  if (neg)
    U->sgn = 1 - U->sgn;

  double y = dint_tod (U);

  return y;
}

#ifndef SECTION
# define SECTION
#endif

#ifndef IN_SINCOS
double
SECTION
__sin (double x)
{
  b64u64_u t = {.f = x};
  int e = (t.u >> 52) & 0x7ff;

  if (__builtin_expect (e == 0x7ff, 0)) /* NaN, +Inf and -Inf. */
    {
#ifdef CORE_MATH_SUPPORT_ERRNO
      if ((t.u << 1) == 0x7ffull<<53){ // Inf
        errno = EDOM;
        return 0.0 / 0.0;
      }
#endif
      if ((t.u << 1) != 0x7ff8ull<<49){
        return 0.0 / 0.0;
      }
      t.u = ~0ull;
      return t.f;
    }

  /* now x is a regular number */

  /* For |x| <= 0x1.7137449123ef6p-26, sin(x) rounds to x (to nearest):
     we can assume x >= 0 without loss of generality since sin(-x) = -sin(x),
     we have x - x^3/6 < sin(x) < x for say 0 < x <= 1 thus
     |sin(x) - x| < x^3/6.
     Write x = c*2^e with 1/2 <= c < 1.
     Then ulp(x)/2 = 2^(e-54), and x^3/6 = c^3/6*2^(3e), thus
     x^3/6 < ulp(x)/2 rewrites as c^3/6*2^(3e) < 2^(e-54),
     or c^3*2^(2e+53) < 3 (1).
     For e <= -26, since c^3 < 1, we have c^3*2^(2e+53) < 2 < 3.
     For e=-25, (1) rewrites 8*c^3 < 3 which yields c <= 0x1.7137449123ef6p-1.
  */
  uint64_t ux = t.u & 0x7fffffffffffffff;
  // 0x3e57137449123ef6 = 0x1.7137449123ef6p-26
  if (ux <= 0x3e57137449123ef6) {
    if (x == 0)
      return x;
    // Taylor expansion of sin(x) is x - x^3/6 around zero
    // for x=-0, fma (x, -0x1p-54, x) returns +0
    /* We have underflow when 0 < |x| < 2^-1022 or when |x| = 2^-1022
       and rounding towards zero. */
    double res = __builtin_fma (x, -0x1p-54, x);
#ifdef CORE_MATH_SUPPORT_ERRNO
    if (__builtin_fabs (x) < 0x1p-1022 || __builtin_fabs (res) < 0x1p-1022)
      errno = ERANGE; // underflow
#endif
    return res;
  }

  double h, l, err;
  err = sin_fast (&h, &l, x);
  double left  = h + (l - err), right = h + (l + err);
  /* With SC[] from ./buildSC 15 we get 1100 failures out of 50000000
     random tests, i.e., about 0.002%. */
  if (__builtin_expect (left == right, 1))
    return left;

  return sin_accurate (x);
}

#ifndef __sin
libm_alias_double (__sin, sin)
#endif

#endif
