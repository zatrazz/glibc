/* Correctly-rounded power function for two binary64 values.

Copyright (c) 2022-2025 CERN and Inria
Authors: Tom Hubrecht and Paul Zimmermann

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

#ifndef CR_POW_H
#define CR_POW_H

#include <stdint.h>
#include <assert.h>

#include <fenv.h>
#include <math.h>
#include <errno.h>
#include "e_pow_data.h"
#include "math_config.h"

/*
  Type definition
*/

static inline int cmpu128 (u128 a, u128 b)
{
  return u128_gt (a, b) - u128_lt (a, b);
}

// Add two 128-bit integers and return 1 if a carry occured
static inline int addu128 (u128 a, u128 b, u128 *r) {
  *r = u128_add (a, b);
  // Return the carry
  return u128_lt (*r, a);
}

// Subtract two 128-bit integers and return 1 if a borrow occured
static inline int subu128 (u128 a, u128 b, u128 *r) {
  *r = u128_sub (a, b);
  // Return the borrow
  return u128_gt (*r, a);
}

typedef union {
  double f;
  uint64_t u;
} f64_u;

// Extract both the mantissa and exponent of a double
static inline void fast_extract (int64_t *e, uint64_t *m, double x) {
  f64_u _x = {.f = x};

  *e = (_x.u >> 52) & 0x7ff;
  *m = (_x.u & (~0ull >> 12)) + (*e ? (1ull << 52) : 0);
  *e = *e - 0x3ff;
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
typedef union {
  u128 r;
  struct {
    uint64_t l;
    uint64_t h;
  };
} uint128_t;
#else
typedef union {
  u128 r;
  struct {
    uint64_t h;
    uint64_t l;
  };
} uint128_t;
#endif

// Add two 128-bit integers and return 1 if a carry occured
static inline int addu_128 (uint128_t a, uint128_t b, uint128_t *r) {
  r->r = u128_add (a.r, b.r);
  // Return the carry
  return u128_lt (r->r, a.r);
}

// Subtract two 128-bit integers and return 1 if a borrow occured
static inline int subu_128 (uint128_t a, uint128_t b, uint128_t *r) {
  r->r = u128_sub (a.r, b.r);
  // Return the borrow
  return u128_gt (r->r, a.r);
}

// Compare two 64-bit signed integers
// Return +1 if a > b, 0 if a=b, -1 if a < b
static inline signed char cmp (int64_t a, int64_t b) {
  return (a > b) - (a < b);
}

// Compare two 64-bit unsigned integers
// Return +1 if a > b, 0 if a=b, -1 if a < b
static inline signed char cmpu (uint64_t a, uint64_t b) {
  return (a > b) - (a < b);
}


#define CORE_MATH_POW
#define CORE_MATH_SUPPORT_ERRNO
#include "dint_pow.h"
#include "qint_pow.h"

/*
  Utility functions
*/

#if 0
// When x is a NaN, returns 1 if x is an sNaN and 0 if it is a qNaN
static inline int issignaling(double x) {
  f64_u _x = {.f = x};

  return !(_x.u & (1ull << 51));
}
#endif

/* Add a + b, such that *hi + *lo approximates a + b.
   Assumes |a| >= |b|.
   For rounding to nearest we have hi + lo = a + b exactly.
   For directed rounding, we have
   (a) hi + lo = a + b exactly when the exponent difference between a and b
       is at most 53 (the binary64 precision)
   (b) otherwise |(a+b)-(hi+lo)| <= 2^-105 min(|a+b|,|hi|)
       (see https://hal.inria.fr/hal-03798376)
   We also have |lo| < ulp(hi). */
static inline void fast_two_sum(double *hi, double *lo, double a, double b) {
  double e;

  // assert (a == 0 || __builtin_fabs (a) >= __builtin_fabs (b));
  *hi = a + b;
  e = *hi - a; /* exact */
  *lo = b - e; /* exact */
}

/* Algorithm 2 from https://hal.science/hal-01351529 */
static inline void two_sum (double *s, double *t, double a, double b)
{
  *s = a + b;
  double a_prime = *s - b;
  double b_prime = *s - a_prime;
  double delta_a = a - a_prime;
  double delta_b = b - b_prime;
  *t = delta_a + delta_b;
}

// Add a + (bh + bl), assuming |a| >= |bh|
static inline void fast_sum(double *hi, double *lo, double a, double bh,
                            double bl) {
  fast_two_sum(hi, lo, a, bh);
  /* |(a+bh)-(hi+lo)| <= 2^-105 |hi| and |lo| < ulp(hi) */
  *lo += bl;
  /* |(a+bh+bl)-(hi+lo)| <= 2^-105 |hi| + ulp(lo),
     where |lo| <= ulp(hi) + |bl|. */
}

// Multiply exactly a and b, such that *hi + *lo = a * b.
static inline void a_mul(double *hi, double *lo, double a, double b) {
  *hi = a * b;
  *lo = __builtin_fma (a, b, -*hi);
}

// Multiply a double with a double double : a * (bh + bl)
static inline void s_mul (double *hi, double *lo, double a, double bh,
                          double bl) {
  double s;

  a_mul (hi, &s, a, bh); /* exact */
  *lo = __builtin_fma (a, bl, s);
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

static inline void d_square(double *hi, double *lo, double ah, double al) {
  double s, b = al + al;

  a_mul(hi, &s, ah, ah);
  *lo = __builtin_fma(ah, b, s);
}

static inline long dtoi(double x) { return (long)x; }

// Returns 1 if x is an integer
static inline int is_int(double x) { return x == roundeven_finite (x); }

// Returns (e, m) such that m is odd and x = 2^E \times m
static inline void extract(int64_t *e, uint64_t *m, double x) {
  f64_u _x = {.f = x};

  *e = (_x.u >> 52) & 0x7ff;
  *m = (_x.u & (~0ull >> 12)) + (*e ? (1ull << 52) : 0);
  int32_t t = __builtin_ctzll(*m);
  *m = *m >> t;
  *e = *e + t - (0x433 - !*e);
}

// Rounds a dint64_t value to 54 bits, a shortcut is taken as in `exact_pow`, we
// only consider numbers that end with only ones or only zeroes
static inline void round_54(int64_t *G, int64_t *k, const dint64_t *x) {
  *G = x->ex - 53;
  *k = (x->hi >> 10) + ((x->hi >> 9) & 0x1);
}

// Multiply x by 2^e
static inline void pow2(double *x, int64_t e) {
  if (e & 0x1)
    *x *= 0x1p+1;

  f64_u e2 = {.u = ((uint64_t)((e >> 1) + 0x3ff) & 0x7ff) << 52};
  *x = (*x * e2.f) * e2.f;
}

// Convert a dint64_t value to an integer, rounding towards zero
static inline int64_t dint_toi(const dint64_t *a) {
  if (a->ex < 0)
    return 0ll;

  int64_t r = a->hi >> (63 - a->ex);

  return a->sgn ? -r : r;
}

// round a, assuming a is in the subnormal range
// exact is non-zero iff x^y is exact
static inline double dint_tod_subnormal(dint64_t *a, int exact) {
  int underflow = 1;
  double ret = 0;

  uint64_t ex = -(1011 + a->ex); // ex >= 12
  // we have to shift right hi,lo by ex bits so that the least significant
  // bit of hi corresponds to 2^-1074 (the number of extra bits is
  // -1022 - a->ex, and we add 11 = 64 - 53 since hi has 64 bits)

  uint64_t rb, sb;

  if (ex >= 64) { // all bits disappear: |a| < 2^-1074
    switch (__fegetround()) {
    case FE_TONEAREST:
      rb = (a->hi >> 63);        // only used when e=64
      sb = (a->hi << 1) | a->lo; // idem
      ret = (ex > 64 || rb == 0 || sb == 0) ? +0.0 : 0x1p-1074;
      ret = (a->sgn) ? -ret : ret;
      break;
    case FE_DOWNWARD:
      ret = (a->sgn) ? -0x1p-1074 : +0.0;
      break;
    case FE_UPWARD:
      ret = (!a->sgn) ? 0x1p-1074 : -0.0;
      break;
    case FE_TOWARDZERO:
      ret = (a->sgn) ? -0.0 : +0.0;
    }
    goto end;
  }

  // now ex < 64
  uint64_t hi;
  hi = a->hi >> ex;
  rb = (a->hi >> (ex - 1)) & 0x1; // round bit
  sb = (a->hi << (65 - ex)) || a->lo; // sticky bit

  switch (__fegetround()) {
  case FE_TONEAREST:
    // if ex=12 there is no underflow when hi rounds to 2^52 and rb=1
    // and the next bit is 1 too
    hi += sb ? rb : hi & rb;
    if (ex == 12 && (hi >> 52) && rb)
    {
      uint64_t rbb = (a->hi >> (ex - 2)) & 0x1; // next bit after the round bit
      if (rbb)
        underflow = 0;
    }
    break;
  case FE_DOWNWARD:
    hi += a->sgn & (sb | rb);
    break;
  case FE_UPWARD:
    // if ex=12 there is no underflow when hi rounds to 2^52 and rb=1
    hi += (!a->sgn) & (sb | rb);
    if (ex == 12 && (hi >> 52) && rb)
      underflow = 0;
    break;
  // for rounding towards zero, don't do anything
  }

  // now hi <= 2^52 stores the low bits of the result (up to sign)
  // (if hi has overflowed in 2^52 this is exactly what we want)

  f64_u v = {.u = hi};
  v.u |= a->sgn << 63;
  ret = v.f;

 end:
  if (underflow && !exact) {
    __feraiseexcept (FE_UNDERFLOW); // raise underflow
#ifdef CORE_MATH_SUPPORT_ERRNO
    errno = ERANGE; // underflow
#endif
  }

  return ret;
}

// Convert a dint64_t value to a double
// exact is non-zero iff x^y is exact
static inline double dint_tod(dint64_t *a, int exact) {
  if (__builtin_expect (a->ex < -1022, 0))
    return dint_tod_subnormal (a, exact);

  // r is the significant in [1,2)
  f64_u r = {.u = (a->hi >> 11) | (0x3ffll << 52)};

  // round r
  double rd = 0.0;
  if ((a->hi >> 10) & 0x1)
    rd += 0x1p-53;

  if (a->hi & 0x3ff || a->lo)
    rd += 0x1p-54;

  if (a->sgn)
    rd = -rd;

  r.u = r.u | a->sgn << 63;
  r.f += rd;

  f64_u e;

  if (a->ex > -1023) { // The result is a normal double
    if (a->ex > 1023) {
      if (a->ex == 1024) { // 2^1024 <= |a| < 2^1025
        r.f = r.f * 0x1p+1;
        e.f = 0x1p+1023;
      } else { // |a| >= 2^1025
        r.f = 0x1.fffffffffffffp+1023;
        e.f = 0x1.fffffffffffffp+1023;
      }
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = ERANGE;
#endif
    }
    else
      e.u = ((a->ex + 1023) & 0x7ff) << 52;
  } else { // subnormal case
    if (!exact) {
      __feraiseexcept (FE_UNDERFLOW); // raise underflow
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = ERANGE; // underflow
#endif
    }
    if (a->ex < -1074) {
      if (a->ex == -1075) {
        r.f = r.f * 0x1p-1;
        e.f = 0x1p-1074;
      } else {
        r.f = 0x0.0000000000001p-1022;
        e.f = 0x0.0000000000001p-1022;
      }
    } else {
      e.u = 1ll << (a->ex + 1074);
    }
  }

#ifdef CORE_MATH_SUPPORT_ERRNO
  if (r.f == 0x1p+1 && e.f == 0x1p+1023)
    errno = ERANGE; // overflow
#endif

  return r.f * e.f;
}

// Convert a double to the corresponding qint64_t value
static inline void qint_fromd (qint64_t *a, double b) {
  fast_extract (&a->ex, &a->hh, b);

  /* |b| = 2^(ex-52)*hi */

  uint32_t t = __builtin_clzll (a->hh);

  a->sgn = b < 0.0;
  a->ex = a->ex - (t > 11 ? t - 12 : 0);
  a->hh = a->hh << t;
  a->lh = 0;
  a->hl = 0;
  a->ll = 0;
  /* b = 2^ex*hh/2^64 where 1 <= hh/2^63 < 2 */
}

// Convert a qint64_t value to an integer
static inline int64_t qint_toi(const qint64_t *a) {
  if (a->ex < 0)
    return 0ll;

  int64_t r = a->hh >> (63 - a->ex);

  return a->sgn ? -r : r;
}

static inline void subnormalize_qint(qint64_t *a) {
  if (a->ex > -1023)
    return;

  uint64_t ex = -(1011 + a->ex);

  uint64_t hi = a->hh >> ex;
  uint64_t md = (a->hh >> (ex - 1)) & 0x1;
  uint64_t lo = (a->hh & (~0ull >> ex)) || a->hl || a->lh || a->ll;

  switch (__fegetround()) {
  case FE_TONEAREST:
    hi += lo ? md : hi & md;
    break;
  case FE_DOWNWARD:
    hi += a->sgn & (md | lo);
    break;
  case FE_UPWARD:
    hi += (!a->sgn) & (md | lo);
    break;
  }

  a->hh = hi << ex;
  a->hl = 0;
  a->lh = 0;
  a->ll = 0;

  if (!a->hh) {
    a->ex++;
    a->hh = (1ull << 63);
  }
}

// Convert a dint64_t value to a double
static inline double qint_tod(qint64_t *a) {
  subnormalize_qint(a);

  f64_u r = {.u = (a->hh >> 11) | (0x3ffll << 52)};

  double rd = 0.0;
  if (a->hh & 0x400)
    rd += 0x1p-53;

  if (a->hh & 0x3ff || a->hl || a->lh || a->ll)
    rd += 0x1p-54;

  if (a->sgn)
    rd = -rd;

  r.u = r.u | a->sgn << 63;
  r.f += rd;

  f64_u e;

  if (a->ex > -1023) { // The result is a normal double
    if (a->ex > 1023)
      if (a->ex == 1024) {
        r.f = r.f * 0x1p+1;
        e.f = 0x1p+1023;
      } else {
        r.f = 0x1.fffffffffffffp+1023;
        e.f = 0x1.fffffffffffffp+1023;
      }
    else
      e.u = ((a->ex + 1023) & 0x7ff) << 52;
  } else { // subnormal case
    __feraiseexcept (FE_UNDERFLOW); // raise underflow
#ifdef CORE_MATH_SUPPORT_ERRNO
    errno = ERANGE; // underflow
#endif
    if (a->ex < -1074) {
      if (a->ex == -1075) {
        r.f = r.f * 0x1p-1;
        e.f = 0x1p-1074;
      } else {
        r.f = 0x0.0000000000001p-1022;
        e.f = 0x0.0000000000001p-1022;
      }
    } else {
      e.u = 1ll << (a->ex + 1074);
    }
  }

  return r.f * e.f;
}

#endif
