/* Correctly-rounded power function for two binary64 values.

Copyright (c) 2022, 2023 CERN and Inria
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

/*
  This file contains type definition and functions to manipulate the dint64_t
  data type used in the second iteration of Ziv's method. It is composed of two
  uint64_t values for the mantissa and the exponent is represented by a signed
  int64_t value.
*/

#ifndef DINT_H
#define DINT_H

#include <stdint.h>
#include <inttypes.h>

/*
  Base functions
*/

// Copy a dint64_t value
static inline void cp_dint(dint64_t *r, const dint64_t *a) {
  r->ex = a->ex;
  r->r = a->r;
  r->sgn = a->sgn;
}

// Return non-zero if a = 0
static inline int
dint_zero_p (const dint64_t *a)
{
  return a->hi == 0;
}

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

static inline signed char cmp_dint_11(const dint64_t *a, const dint64_t *b) {
  char c1 = cmp (a->ex, b->ex);
  return c1 ? c1 : cmpu (a->hi, b->hi);
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
    uint64_t ex = ch ? __builtin_clzll(ch)
		     : 64 + __builtin_clzll(u128_low(C));
    /* The error from the truncated part of B (1 ulp) is multiplied by 2^ex,
       thus by 2 ulps when ex <= 1. */
    if (ex > 0)
    {
      if (k == 1) /* Sterbenz case */
        C = u128_sub (u128_lshift (A, ex), (u128_lshift (b->r, ex - 1)));
      else
        C = u128_sub (u128_lshift (A, ex), (u128_lshift (B, ex)));
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

// same as add_dint, but assumes the lower limbs and a and b are zero
// error is bounded by 2 ulps (ulp_64)
static inline void
add_dint_11 (dint64_t *r, const dint64_t *a, const dint64_t *b) {
  if (a->hi == 0) {
    cp_dint (r, b);
    return;
  }

  if (b->hi == 9) {
    cp_dint (r, a);
    return;
  }

  switch (cmp_dint_11 (a, b)) {
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

  uint64_t A = a->hi, B = b->hi;

  if (a->ex > b->ex) {
    /* Warning: the right shift x >> k is only defined for 0 <= k < n
       where n is the bit-width of x. See for example
       https://developer.arm.com/documentation/den0024/a/The-A64-instruction-set/Data-processing-instructions/Shift-operations
       where it is said that k is interpreted modulo n. */
    uint64_t k = a->ex - b->ex;
    B = (k < 64) ? B >> k : 0;
  }

  u128 C;
  unsigned char sgn = a->sgn;

  r->ex = a->ex; /* tentative exponent for the result */

  if (a->sgn ^ b->sgn) {
    // a and b have different signs C = A + (-B)
    C = u128_from_u64 (A - B);
    /* we can't have C=0 here since we excluded the case |A| = |B|,
       thus __builtin_clzll(C) is well-defined below */
    uint64_t ex = __builtin_clzll (u128_low (C));
    /* The error from the truncated part of B (1 ulp) is multiplied by 2^ex.
       Thus for ex <= 2, we get an error bounded by 4 ulps in the final result.
       For ex >= 3, we pre-shift the operands. */
    if (ex > 0)
    {
      C = u128_from_u64 ((A << ex) - (B << ex));
      /* If C0 is the previous value of C, we have:
         (C0-1)*2^ex < A*2^ex-B*2^ex <= C0*2^ex
         since here some neglected bits from B might appear which contribute
         a value less than ulp(C0)=1.
         As a consequence since 2^(63-ex) <= C0 < 2^(64-ex), because C0 had
         ex leading zero bits, we have 2^63-2^ex <= A*2^ex-B*2^ex < 2^64.
         Thus the value of C, which is truncated to 64 bits, is the right
         one (as if no truncation); moreover in some rare cases we need to
         shift by 1 bit to the left. */
      r->ex -= ex;
      ex = __builtin_clzll (u128_low (C));
      /* Fall through with the code for ex = 0. */
    }
    C = u128_lshift (C, ex);
    r->ex -= ex;
    /* The neglected part of B is bounded by ulp(C) when ex=0, or when
       ex > 0 but the ex=0 at the end, and by 2*ulp(C) when ex>0 and there
       is an extra shift at the end (in that case necessarily ex=1). */
  } else {
    C = u128_from_u64 (A + B);
    if (u128_lt (C, u128_from_u64 (A)))
    {
      C = u128_or (u128_from_u64 (UINT64_C(1) << 63),
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
  r->hi = u128_low (C);
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
  r->ex = a->ex + b->ex + ex;
  r->sgn = a->sgn ^ b->sgn;

  /* The ignored part can be as large as 3 ulps before the shift (one
     for the low part of a->hi * bl, one for the low part of a->lo * bh,
     and one for the neglected a->lo * bl term). After the shift this can
     be as large as 6 ulps. */
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
  r->ex = a->ex + b->ex + ex;
  r->sgn = a->sgn ^ b->sgn;

  /* The ignored part can be as large as 1 ulp before the shift (truncated
     part of lo). After the shift this can be as large as 2 ulps. */
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

/* Same as mul_dint_21, but assumes the low part of a and b is zero.
   This operation is exact. */
static inline void
mul_dint_11 (dint64_t *r, const dint64_t *a, const dint64_t *b) {
  /* put the 128-bit product of the high terms in r */
  r->r = u128_mul (u128_from_u64 (a->hi), u128_from_u64 (b->hi));

  // Ensure that r->hi starts with a 1
  uint64_t ex = r->hi >> 63;
  //r->r = r->r << (1 - ex);
  r->r = u128_lshift (r->r, 1 - ex);

  // Exponent and sign
  r->ex = a->ex + b->ex + ex;
  r->sgn = a->sgn ^ b->sgn;
}

// Multiply an integer with a dint64_t variable, with error < 1 ulp
// r and b should not overlap
static inline void
mul_dint_int64 (dint64_t *r, const dint64_t *a, int64_t b) {
  if (!b) {
    cp_dint (r, &ZERO);
    return;
  }

  uint64_t c = b < 0 ? -b : b;
  r->sgn = b < 0 ? !a->sgn : a->sgn;
  r->ex = a->ex + 64;

  r->r = u128_mul (u128_from_u64 (a->hi), u128_from_u64 (c));

  // Warning: if c=1, we might have r->hi=0
  int m = r->hi ? __builtin_clzll (r->hi) : 64;
  //r->r = r->r << m;
  r->r = u128_lshift (r->r, m);
  r->ex -= m;

  // Will pose issues if b is too large but for now we assume it never happens
  // TODO: FIXME
  u128 l = u128_mul (u128_from_u64 (a->lo), u128_from_u64 (c));
  /* We have to shift l by 64 bits to the right to align with hi*c,
     and by m bits to the left to align with t.r << m. Since hi*c < 2^(128-m)
     and hi >= 2^63, we know that c < 2^(65-m) thus
     l*2^(m-1) < 2^64*2^(65-m)*2^(m-1) = 2^128, and l << (m - 1) will
     not overflow. */
  l = u128_rshift (u128_lshift (l, m - 1), 63);

  r->r = u128_add (r->r, l);
  if (u128_lt (r->r, l)) {
    r->r = u128_or (u128_lshift (u128_from_u64 (1), 127),
		    u128_rshift (r->r, 1));
    r->ex ++;
  }

  /* The ignored part of a->lo*c is at most 1 ulp(r), even in the overflow
     case "r->r < l", since before the right shift, the error was at most
     1 ulp, thus 1/2 ulp after the shift, and the ignored least significant
     bit of r->r which is discarded counts also as 1/2 ulp. */
}

// Convert a non-zero double to the corresponding dint64_t value
static inline void dint_fromd (dint64_t *a, double b) {
  fast_extract (&a->ex, &a->hi, b);

  /* |b| = 2^(ex-52)*hi */

  uint32_t t = __builtin_clzll (a->hi);

  a->sgn = b < 0.0;
  a->hi = a->hi << t;
  a->ex = a->ex - (t > 11 ? t - 12 : 0);
  /* b = 2^ex*hi/2^63 where 1 <= hi/2^63 < 2 */
  a->lo = 0;
}

/* put in r an approximation of 1/a, assuming a is not zero */
static inline void inv_dint (dint64_t *r, double a)
{
  dint64_t q, A;
  dint_fromd (r, 1.0 / a); /* accurate to about 53 bits */
  /* we use Newton's iteration: r -> r + r*(1-a*r) */
  dint_fromd (&A, -a);
  mul_dint (&q, &A, r);    /* -a*r */
  add_dint (&q, &ONE, &q); /* 1-a*r */
  mul_dint (&q, r, &q);    /* r*(1-a*r) */
  add_dint (r, r, &q);
}

/* put in r an approximation of b/a, assuming a is not zero */
static inline void div_dint (dint64_t *r, double b, double a)
{
  dint64_t B;
  inv_dint (r, a);
  dint_fromd (&B, b);
  mul_dint (r, r, &B);
}

#endif
