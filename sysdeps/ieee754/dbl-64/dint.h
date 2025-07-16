/* Helper functions for cos/sin implementation

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

#ifndef _DINT_H
#define _DINT_H

#include <math_uint128.h>
#include <rounding-mode.h>

typedef union {
  struct {
    u128 r;
    int64_t _ex;
    uint64_t _sgn;
  };
  struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint64_t lo;
    uint64_t hi;
#else
    uint64_t hi;
    uint64_t lo;
#endif
    int64_t ex;
    uint64_t sgn;
  };
} dint64_t;

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

typedef union {
  double f;
  uint64_t u;
} f64_u;

// add two 128 bit integers and return 1 if an overflow occured
static inline int addu_128(uint128_t a, uint128_t b, uint128_t *r) {
  r->l = a.l + b.l;
  r->h = a.h + b.h + (r->l < a.l);

  // return the overflow
  return r->h == a.h ? r->l < a.l : r->h < a.h;
}

// Return non-zero if a = 0
static inline int
dint_zero_p (const dint64_t *a)
{
  return a->hi == 0;
}

static inline int cmp(int64_t a, int64_t b) { return (a > b) - (a < b); }

static inline int cmpu128(u128 a, u128 b) { return u128_gt (a, b) - u128_lt (a, b); }

/* ZERO is a dint64_t representation of 0, which ensures that
   dint_tod(ZERO) = 0 */
static const dint64_t ZERO = {.hi = 0x0, .lo = 0x0, .ex = -1076, .sgn = 0x0};
/* ONE is a dint64_t representation of 1 */
static const dint64_t ONE = {
    .hi = 0x8000000000000000, .lo = 0x0, .ex = 1, .sgn = 0x0};

static const dint64_t ONE_2 = {
    .hi = 0x8000000000000000, .lo = 0x0, .ex = 0, .sgn = 0x0};

static const dint64_t M_ONE = {
    .hi = 0x8000000000000000, .lo = 0x0, .ex = 0, .sgn = 0x1};

/* the following is an approximation of log(2), with absolute error less
   than 2^-129.97 */
static const dint64_t LOG2 = {
    .hi = 0xb17217f7d1cf79ab, .lo = 0xc9e3b39803f2f6af, .ex = -1, .sgn = 0x0};
// MAGIC is a dint64_t representation of 1/2^11
static const dint64_t MAGIC = {.hi = 0x8000000000000000, .lo = 0x0, .ex = -10, .sgn = 0x0};

/* the following is an approximation of 2^12/log(2), with absolute error less
   than 2^-118.63: |(hi/2^63+lo/2^127)*2^12 - 2^12/log(2)| < 2^-118.63 */
static const dint64_t LOG2_INV = {
    .hi = 0xb8aa3b295c17f0bb, .lo = 0xbe87fed0691d3e89, .ex = 12, .sgn = 0x0};

/* the following is an approximation of 1/log(10), with absolute error
   less than 2^-131.72 */
static const dint64_t ONE_OVER_LOG10 = {
  .hi = 0xde5bd8a937287195, .lo = 0x355baaafad33dc32, .ex = -2, .sgn = 0x0};

/* the following is an approximation of 1/log(10), with absolute error less
   than 2^-118.63: |(hi/2^63+lo/2^127)*2^-2 - 1/log(10)| < 2^-131.02 */
static const dint64_t LOG10_INV = {
    .hi = 0xde5bd8a937287195, .lo = 0x355baaafad33dc32, .ex = -2, .sgn = 0x0};

extern const dint64_t __dint_INVERSE_2[] attribute_hidden;
#define _INVERSE_2 __dint_INVERSE_2
extern const dint64_t __dint_LOG_INV_2[] attribute_hidden;
#define _LOG_INV_2 __dint_LOG_INV_2
extern const dint64_t __dint_P_2[] attribute_hidden;
#define P_2 __dint_P_2

// Compare the absolute values of a and b
// Return -1 if |a| < |b|
// Return  0 if |a| = |b|
// Return +1 if |a| > |b|
static inline signed char
cmp_dint_abs (const dint64_t *a, const dint64_t *b) {
  if (dint_zero_p (a))
    return dint_zero_p (b) ? 0 : -1;
  if (dint_zero_p (b))
    return +1;
  char c1 = cmp (a->ex, b->ex);
  return c1 ? c1 : cmpu128 (a->r, b->r);
}

// Copy a dint64_t value
static inline void cp_dint(dint64_t *r, const dint64_t *a) {
  r->ex = a->ex;
  r->r = a->r;
  r->sgn = a->sgn;
}

// Add two dint64_t values, with error bounded by 2 ulps (ulp_128)
// (more precisely 1 ulp when a and b have same sign, 2 ulps otherwise)
// Moreover, when Sterbenz theorem applies, i.e., |b| <= |a| <= 2|b|
// and a,b are of different signs, there is no error, i.e., r = a-b.
static inline void
add_dint (dint64_t *r, const dint64_t *a, const dint64_t *b) {
  if (!(a->hi | a->lo)) {
    cp_dint (r, b);
    return;
  }

  switch (cmp_dint_abs (a, b)) {
  case 0:
    if (a->sgn ^ b->sgn) {
      cp_dint (r, &ZERO);
      return;
    }

    cp_dint (r, a);
    r->ex++;
    return;

  case -1: // |A| < |B|
    {
      // swap operands
      const dint64_t *tmp = a; a = b; b = tmp;
      break; // fall through the case |A| > |B|
    }
  }

  // From now on, |A| > |B| thus a->ex >= b->ex

  u128 A = a->r, B = b->r;
  uint64_t k = a->ex - b->ex;

  if (k > 0) {
    /* Warning: the right shift x >> k is only defined for 0 <= k < n
       where n is the bit-width of x. See for example
       https://developer.arm.com/documentation/den0024/a/The-A64-instruction-set/Data-processing-instructions/Shift-operations
       where it is said that k is interpreted modulo n. */
    B = (k < 128) ? u128_rshift (B, k) : u128_from_u64 (0);
  }

  u128 C;
  unsigned char sgn = a->sgn;

  r->ex = a->ex; /* tentative exponent for the result */

  if (a->sgn ^ b->sgn) {
    /* a and b have different signs C = A + (-B)
       Sterbenz case |a|/2 <= |b| <= |a| can occur only when:
       * k=0: then B is not truncated, and C is exact below
       * k=1 and ex>0 below: then we ensure C is exact
     */
    C = u128_sub (A, B);
    uint64_t ch = u128_high (C);
    /* We can't have C=0 here since we excluded the case |A| = |B|,
       thus __builtin_clzll(C) is well-defined below. */
    uint64_t ex = ch ? __builtin_clzll(ch) : 64 + __builtin_clzll(u128_low (C));
    /* The error from the truncated part of B (1 ulp) is multiplied by 2^ex,
       thus by 2 ulps when ex <= 1. */
    if (ex > 0)
    {
      if (k == 1) /* Sterbenz case */
	C = u128_sub (u128_lshift (A, ex), u128_lshift (b->r, ex - 1));
      else
	C = u128_sub (u128_lshift (A, ex), u128_lshift (B, ex));
      /* If C0 is the previous value of C, we have:
         (C0-1)*2^ex < A*2^ex-B*2^ex <= C0*2^ex
         since some neglected bits from B might appear which contribute
         a value less than ulp(C0)=1.
         As a consequence since 2^(127-ex) <= C0 < 2^(128-ex), because C0 had
         ex leading zero bits, we have 2^127-2^ex <= A*2^ex-B*2^ex < 2^128.
         Thus the value of C, which is truncated to 128 bits, is the right
         one (as if no truncation); moreover in some rare cases we need to
         shift by 1 bit to the left. */
      r->ex -= ex;
      ex = __builtin_clzll (u128_high (C));
      /* Fall through with the code for ex = 0. */
    }
    C = u128_lshift (C, ex);
    r->ex -= ex;
    /* The neglected part of B is bounded by 2 ulp(C) when ex=0, 1 ulp
       when ex > 0 but ex=0 at the end, and by 2*ulp(C) when ex > 0 and there
       is an extra shift at the end (in that case necessarily ex=1). */
  } else {
    C = u128_add (A, B);
    if (u128_lt (C, A))
    {
      C = u128_or (u128_lshift (u128_from_u64 (1), 127),
		   u128_rshift (C, 1));
      r->ex ++;
    }
  }

  /* In the addition case, we loose the truncated part of B, which
     contributes to at most 1 ulp. If there is an exponent shift, we
     might also loose the least significant bit of C, which counts as
     1/2 ulp, but the truncated part of B is now less than 1/2 ulp too,
     thus in all cases the error is less than 1 ulp(r). */

  r->sgn = sgn;
  r->r = C;
}

// Multiply two dint64_t numbers, with error bounded by 6 ulps
// on the 128-bit floating-point numbers.
// Overlap between r and a is allowed
static inline void
mul_dint (dint64_t *r, const dint64_t *a, const dint64_t *b) {
  u128 bh = u128_from_u64 (b->hi), bl = u128_from_u64 (b->lo);

  /* compute the two middle terms */
  u128 m1 = u128_mul (u128_from_u64 (a->hi), bl);
  u128 m2 = u128_mul (u128_from_u64 (a->lo), bh);

  /* put the 128-bit product of the high terms in r */
  r->r = u128_mul (u128_from_u64 (a->hi), bh);

  /* there can be no overflow in the following addition since r <= (B-1)^2
     with B=2^64, (m1>>64) <= B-1 and (m2>>64) <= B-1, thus the sum is
     bounded by (B-1)^2+2*(B-1) = B^2-1 */
  r->r = u128_add (r->r, u128_add (u128_from_u64 (u128_high (m1)),
				   u128_from_u64 (u128_high (m2))));

  // Ensure that r->hi starts with a 1
  uint64_t ex = r->hi >> 63;
  r->r = u128_lshift (r->r, 1 - ex);

  // Exponent and sign
  // if ex=1, then ex(r) = ex(a) + ex(b)
  // if ex=0, then ex(r) = ex(a) + ex(b) - 1
  r->ex = a->ex + b->ex + ex - 1;
  r->sgn = a->sgn ^ b->sgn;

  /* The ignored part can be as large as 3 ulps before the shift (one
     for the low part of a->hi * bl, one for the low part of a->lo * bh,
     and one for the neglected a->lo * bl term). After the shift this can
     be as large as 6 ulps. */
}

// Multiply two dint64_t numbers, with 126 bits of accuracy
static inline void
mul_dint_126 (dint64_t *r, const dint64_t *a, const dint64_t *b) {
  uint128_t t = {.r = u128_mul(u128_from_u64(a->hi), u128_from_u64(b->hi))};
  uint128_t m1 = {.r = u128_mul(u128_from_u64(a->hi), u128_from_u64(b->lo))};
  uint128_t m2 = {.r = u128_mul(u128_from_u64(a->lo), u128_from_u64(b->hi))};

  uint128_t m;
  // If we only garantee 127 bits of accuracy, we improve the simplicity of the
  // code uint64_t l = ((u128)(a->lo) * (u128)(b->lo)) >> 64; m.l += l; m.h +=
  // (m.l < l);
  t.h += addu_128(m1, m2, &m);
  t.r = u128_add (t.r, u128_from_u64 (m.h));

  // Ensure that r->hi starts with a 1
  uint64_t ex = !(t.h >> 63);
  if (ex)
    t.r = u128_lshift (t.r, 1);

  //t.r += (m.l >> 63);
  t.r = u128_add (t.r, u128_from_u64 (m.l >> 63));

  r->hi = t.h;
  r->lo = t.l;

  // Exponent and sign
  r->ex = a->ex + b->ex - ex + 1;
  r->sgn = a->sgn ^ b->sgn;
}

// Multiply an integer with a dint64_t variable
static inline void mul_dint_2(dint64_t *r, int64_t b, const dint64_t *a) {
  uint128_t t;

  if (!b) {
    cp_dint(r, &ZERO);
    return;
  }

  uint64_t c = b < 0 ? -b : b;
  r->sgn = b < 0 ? !a->sgn : a->sgn;

  t.r = u128_mul (u128_from_u64 (a->hi), u128_from_u64 (c));

  int m = t.h ? __builtin_clzll(t.h) : 64;
  t.r = u128_lshift (t.r, m);

  // Will pose issues if b is too large but for now we assume it never happens
  // TODO: FIXME
  uint128_t l = {.r = u128_mul (u128_from_u64 (a->lo), u128_from_u64 (c))};
  l.r = u128_rshift (u128_lshift (l.r, m - 1), 63);

  if (addu_128(l, t, &t)) {
    t.r = u128_add (t.r, u128_bitwise_and (t.r, u128_from_u64 (0x1)));
    t.r = u128_bitwise_or (u128_lshift (u128_from_u64 (1), 127),
			   u128_rshift (t.r, 1));
    m--;
  }

  r->hi = t.h;
  r->lo = t.l;
  r->ex = a->ex + 64 - m;
}

// Multiply two dint64_t numbers, assuming the low part of b is zero
// with error bounded by 2 ulps
static inline void
mul_dint_21 (dint64_t *r, const dint64_t *a, const dint64_t *b) {
  u128 bh = u128_from_u64 (b->hi);
  u128 hi = u128_mul (u128_from_u64 (a->hi), bh);
  u128 lo = u128_mul (u128_from_u64 (a->lo), bh);

  /* put the 128-bit product of the high terms in r */
  r->r = hi;

  /* add the middle term */
  r->r = u128_add (r->r, u128_from_u64 (u128_high (lo)));

  // Ensure that r->hi starts with a 1
  uint64_t ex = r->hi >> 63;
  r->r = u128_lshift (r->r, 1 - ex);

  // Exponent and sign
  r->ex = a->ex + b->ex + ex - 1;
  r->sgn = a->sgn ^ b->sgn;

  /* The ignored part can be as large as 1 ulp before the shift (truncated
     part of lo). After the shift this can be as large as 2 ulps. */
}


/* put in r an approximation of 1/a, assuming a is not zero,
   with error bounded by 4.003 ulps (relative error < 2^-124.999) */
static inline void inv_dint (dint64_t *r, dint64_t *a)
{
  extern const uint64_t __dint_Tinv[256] attribute_hidden;
#define Tinv __dint_Tinv
  uint64_t h = a->hi; /* 2^63 <= h < 2^64 */
  /* First compute a 64-bit inverse t of h, such that
     t*h ~ 2^127, see routine inv_dint() in tan.sage.
     We note a = h/2^63, then 1 <= a < 2, and we write x = t/2^64 for
     the approximation of 1/a, with 1/2 <= x < 1. */
  int i = (h>>55) & 0xff;
  uint64_t t = Tinv[i];
  /* now t is accurate to about 8 bits, more precisely the integer residual
     2^127 - h*t is bounded by 662027951208051476078044039717322752 < 2^118.995
     (attained for i=0 and h=2^63). The integer residual 2^127 - t*h
     equals 2^127*(1-a*x), thus 0 <= 1-ax < 2^-8.005. */

  /* first Newton iteration */
  // exact
  u128 e = u128_sub (u128_lshift (u128_from_u64 (1), 127),
		     u128_mul (u128_from_u64 (h), u128_from_u64 (t)));
  /* as Tinv was computed, we have 0 < e < 2^119 */
  e = u128_mul (u128_from_u64 (t), u128_rshift (e, 55));
  t = u128_low (u128_add (u128_from_u64 (t), u128_rshift (e, 72)));
  /* If we had no truncation, the residual 1-ax is squared at each iteration,
     thus we would get 2^127 - h*t < 2^(2*118.995)/2^127 <= 2^110.99.
     The truncation e>>55 induces an error of at most t < 2^64 on the
     value of e after e = (u128) t * (e >> 55), thus of at most 2^-8 on t,
     while the truncation e >> 72 induces an error of at most 1 on t,
     thus the error on t is at most 1+2^-8.
     Since h < 2^64, this can increase by at most 2^64*(1+2^-8) the value
     of 2^127 - h*t:
     2^127 - h*t < 2^110.99 + 2^64*(1+2^-8) < 2^110.991.
  */

  /* second Newton iteration */
  // 0 <= e < 2^111
  e = u128_sub (u128_lshift (u128_from_u64 (1), 127),
		u128_mul (u128_from_u64 (h), u128_from_u64 (t)));
  e = u128_mul (u128_from_u64 (t), u128_rshift (e, 47));
  t = u128_low (u128_add (u128_from_u64 (t), u128_rshift (e, 80)));
  /* With the same reasoning as above, the truncation e >> 47 induces an
     error of at most 2^-16 on t, and the truncation e >> 80 an error of
     at most 1, giving a total of 1+2^-16.
     Since h < 2^64, this can increase by at most 2^64*(1+2^-16) the value
     of 2^127 - h*t:
     2^127 - h*t < 2^(2*110.991)/2^127 + 2^64*(1+2^-16) < 2^94.9821. */
     

  /* third Newton iteration */
  // 0 <= e < 2^95
  e = u128_sub (u128_lshift (u128_from_u64 (1), 127),
		u128_mul (u128_from_u64 (h), u128_from_u64 (t)));
  e = u128_mul (u128_from_u64 (t), u128_rshift (e, 31));
  t = u128_low (u128_add (u128_from_u64 (t), u128_rshift (e, 96)));
  /* With the same reasoning as above, the truncation e >> 31 induces an
     error of at most 2^-32 on t, and the truncation e >> 96 an error of
     at most 1, giving a total of 1+2^-32.
     Since h < 2^64, this can increase by at most 2^64*(1+2^-32) the value
     of 2^127 - h*t:
     2^127 - h*t < 2^(2*94.9821)/2^127 + 2^64*(1+2^-32) < 2^64.574.
     This corresponds to:
     1 - a*x < 2^64.574/2^127 = 2^-62.426, thus the relative error is at
     most 2^-62.426. */
  
  dint64_t q[1];
  r->hi = t;
  r->lo = 0;
  /* if a->ex = 0, then 1/2 <= a < 1, thus we should have
     1 < 1/a <= 2, thus r->ex = 1 */
  r->ex = 1 - a->ex;
  r->sgn = 1;
  /* we use Newton's iteration: r -> r + r*(1-a*r) */
  mul_dint_21 (q, a, r);     /* -a*r, error <= 2 ulps */
  r->sgn = 0;                /* restore sign */
  add_dint (q, &ONE, q);     /* 1-a*r, error <= 2 ulps */
  mul_dint (q, r, q);        /* r*(1-a*r), error <= 6 ulps */
  add_dint (r, r, q);        /* error <= 2 ulps */
  /* If all computations were exact, the residual would be squared,
     thus we would get 1-a*r = 2^(-2*62.426) = 2^-124.852.
     To simplify the error analysis, we assume 1 <= a < 2 and thus
     1/2 <= r <= 1.
     * since |ar| <= 1, the error of at most 2 ulps from mul_dint_21()
       translates to an absolute error of at most 2^-127; this error
       is multiplied by r <= 1, thus contributes to at most 2^-127 in the
       final value of r.
     * since 1-a*r < 2^-62.426, the error of at most 2 ulps in
       add_dint (&q, &ONE, &q) translates to an absolute error of at most
       2^-189; this error is multiplied by r <= 1, thus contributes to at most
       2^-189 in the final value of r.
     * since q < 2^-62.426 and r < 1, the value of q after mul_dint()
       satisfies q < 2^-62.426, thus the error of at most 6 ulps translates
       into an absolute error of at most 2^-187 in the final value of r.
     * since r <= 1, the error of at most 2 ulps in add_dint (r, r, &q)
       translates into an absolute error of at most 2^-127 in the final
       value of r.
     This yields a maximal absolute error of 2^-127+2^-189+2^-187+2^-127
     < 2^-125.999. Since r >= 1/2, this corresponds to a relative error
     bounded by 2^-124.999, or to less than 4.003 ulps, since in the
     binade [1/2,1), the ulp is 2^-128.
  */
#undef Tinv
}

/* put in r an approximation of b/a, assuming a is not zero,
   with relative error < 2^-123.67 */
static inline void div_dint (dint64_t *r, dint64_t *b, dint64_t *a)
{
  inv_dint (r, a);    // relative error bounded by 2^-124.999
  mul_dint (r, r, b); // error bounded by 6 ulps
  /* The error bound of 6 ulps for mul_dint() corresponds to a maximal
     error of 6*2^-128 in the binade [1/2,1), thus to a maximal relative
     error of 12*2^-128:
     r = b/a * (1 + eps1) * (1 + eps2)
     with |eps1| < 2^-124.999 and |eps2| < 12*2^-128,
     thus r = b/a * (1 + eps) with eps < (1 + 2^-124.999) * (1 + 12*2^-128) - 1
     < 2^-123.67. */
}


// Extract both the mantissa and exponent of a double
static inline void fast_extract (int64_t *e, uint64_t *m, double x, int bias) {
  f64_u _x = {.f = x};

  *e = (_x.u >> 52) & 0x7ff;
  *m = (_x.u & (~0ull >> 12)) + (*e ? (1ull << 52) : 0);
  *e = *e - bias;
}

// Convert a non-zero double to the corresponding dint64_t value
static inline void dint_fromd (dint64_t *a, double b, int bias) {
  fast_extract (&a->ex, &a->hi, b, bias);

  /* |b| = 2^(ex-52)*hi */

  uint32_t t = __builtin_clzll (a->hi);

  a->sgn = b < 0.0;
  a->hi = a->hi << t;
  a->ex = a->ex - (t > 11 ? t - 12 : 0);
  /* b = 2^ex*hi/2^64 where 1/2 <= hi/2^64 < 1 */
  a->lo = 0;
}

/* put in r an approximation of 1/a, assuming a is not zero */
static inline void inv_dint_d (dint64_t *r, double a)
{
  dint64_t q, A;
  // we convert 4/a and divide by 4 to avoid a spurious underflow
  dint_fromd (r, 4.0 / a, 0x3ff); /* accurate to about 53 bits */
  r->ex -= 2;
  /* we use Newton's iteration: r -> r + r*(1-a*r) */
  dint_fromd (&A, -a, 0x3ff);
  mul_dint_126 (&q, &A, r);    /* -a*r */
  add_dint (&q, &ONE_2, &q); /* 1-a*r */
  mul_dint_126 (&q, r, &q);    /* r*(1-a*r) */
  add_dint (r, r, &q);
}

/* put in r an approximation of b/a, assuming a is not zero */
static inline void div_dint_d (dint64_t *r, double b, double a)
{
  dint64_t B;
  inv_dint_d (r, a);
  dint_fromd (&B, b, 0x3ff);
  mul_dint_126 (r, r, &B);
}

static inline void subnormalize_dint(dint64_t *a) {
  if (a->ex > -1023)
    return;

  uint64_t ex = -(1011 + a->ex);

  uint64_t hi = a->hi >> ex;
  uint64_t md = (a->hi >> (ex - 1)) & 0x1;
  uint64_t lo = (a->hi & (~0ull >> ex)) || a->lo;

  switch (get_rounding_mode()) {
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

  a->hi = hi << ex;
  a->lo = 0;

  if (!a->hi) {
    a->ex++;
    a->hi = (1ull << 63);
  }
}

// Convert a dint64_t value to a double
static inline double dint_tod(dint64_t *a) {
  subnormalize_dint (a);

  f64_u r = {.u = (a->hi >> 11) | (0x3ffll << 52)};

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

  if (a->ex > -1022) { // The result is a normal double
    if (a->ex > 1024)
      if (a->ex == 1025) {
        r.f = r.f * 0x1p+1;
        e.f = 0x1p+1023;
      } else {
        r.f = 0x1.fffffffffffffp+1023;
        e.f = 0x1.fffffffffffffp+1023;
      }
    else
      e.u = ((a->ex + 1022) & 0x7ff) << 52;
  } else {
    if (a->ex < -1073) {
      if (a->ex == -1074) {
        r.f = r.f * 0x1p-1;
        e.f = 0x1p-1074;
      } else {
        r.f = 0x0.0000000000001p-1022;
        e.f = 0x0.0000000000001p-1022;
      }
    } else {
      e.u = 1l << (a->ex + 1073);
    }
  }

  return r.f * e.f;
}

// Convert a dint64_t value to a double
// assuming the input is not in the subnormal range
static inline double dint_tod_not_subnormal (dint64_t *a) {

  f64_u r = {.u = (a->hi >> 11) | (0x3ffll << 52)};
  /* r contains the upper 53 bits of a->hi, 1 <= r < 2 */

  double rd = 0.0;
  /* if round bit is 1, add 2^-53 */
  if ((a->hi >> 10) & 0x1)
    rd += 0x1p-53;

  /* if trailing bits after the rounding bit are non zero, add 2^-54 */
  if (a->hi & 0x3ff || a->lo)
    rd += 0x1p-54;

  r.u = r.u | a->sgn << 63;
  r.f += (a->sgn == 0) ? rd : -rd;

  f64_u e;

  /* For log, the result is always in the normal range,
     thus a->ex > -1023. Similarly, we cannot have a->ex > 1023. */

  e.u = ((a->ex + 1023) & 0x7ff) << 52;

  return r.f * e.f;
}

#endif
