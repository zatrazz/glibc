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
  This file contains type definition and functions to manipulate the qint64_t
  data type used in the third iteration of Ziv's method. It is composed of four
  uint64_t values for the mantissa and the exponent is represented by a signed
  int64_t value.
*/

#ifndef QINT_H
#define QINT_H

#include <stdint.h>
#include <inttypes.h>

/*
  Base functions
*/

// Copy a qint64_t value
static inline void cp_qint (qint64_t *r, const qint64_t *a) {
  r->ex = a->ex;
  r->rh = a->rh;
  r->rl = a->rl;
  r->sgn = a->sgn;
}

/* Compare the absolute values of a and b:
   return +1 if |a| > |b|, 0 if |a| = |b|, -1 if |a| < |b| */
static inline signed char cmp_qint(const qint64_t *a, const qint64_t *b) {
  return cmp(a->ex, b->ex)  ? cmp(a->ex, b->ex)
    : cmpu128(a->rh, b->rh) ? cmpu128(a->rh, b->rh)
    : cmpu128(a->rl, b->rl);
}

/* same as cmp_qint, but only compare the upper 2 limbs */
static inline signed char cmp_qint_22(const qint64_t *a, const qint64_t *b) {
  return cmp(a->ex, b->ex)  ? cmp(a->ex, b->ex)
    : cmpu128(a->rh, b->rh);
}

/* Add two qint64_t values, with error bounded by 2 ulps (ulp_256).
   If Sterbenz theorem applies, i.e., a and b are of opposite signs
   with |a|/2 <= |b| <= |a|, then the operation is exact. */
static inline void
add_qint (qint64_t *r, const qint64_t *a, const qint64_t *b) {
  if (u128_eq (a->rh, u128_from_u64 (0))
      && u128_eq (a->rl, u128_from_u64 (0))) {
    cp_qint (r, b); // exact
    return;
  }

  if (u128_eq (b->rh, u128_from_u64 (0))
      && u128_eq (b->rl, u128_from_u64 (0))) {
    cp_qint (r, a); // exact
    return;
  }

  /* compare the absolute values of a and b */
  switch (cmp_qint (a, b)) {
  case 0: /* |a| = |b| */
    if (a->sgn ^ b->sgn) { /* signs differ */
      cp_qint (r, &ZERO_Q); // exact
      return;
    }

    cp_qint (r, a);
    r->ex++; // exact
    return;

  case -1: /* |a| < |b| */
    add_qint (r, b, a);
    return;
  }

  // From now on, |A| > |B|

  u128 ah = a->rh, al = a->rl, bh = b->rh, bl = b->rl;

  int64_t m_ex = a->ex;
  int64_t k = a->ex - b->ex;

  if (k > 0) {
    if (k >= 128) {
      bl = (k < 256) ? u128_rshift (bh, k - 128) : u128_from_u64 (0);
      bh = u128_from_u64 (0);
    } else { /* 1 <= k <= 127 */
      bl = u128_or (u128_rshift (bl, k), u128_lshift (bh, 128 - k));
      bh = u128_rshift (bh, k);
    }
  }

  /* now we have to add (ah,al) + (bh,bl), with error <= 1 ulp
     corresponding to the ignored part of (bh,bl) */

  unsigned char sgn = a->sgn;
  uint64_t ex;
  u128 ch, cl;

  r->ex = m_ex;

  if (a->sgn ^ b->sgn) { // subtraction case
    /* a and b have different signs: C = A + (-B) */
    ch = u128_sub (ah, bh);

    if (subu128 (al, bl, &cl))
      ch = u128_sub (ch, u128_from_u64 (1));
    /* we cannot have C=0 since |A| > |B| */
    uint64_t chh = u128_high (ch), clh = u128_high (cl);
    ex =
      chh ? __builtin_clzll(chh)
      : 64 + (!u128_eq (ch, u128_from_u64(0)) ? __builtin_clzll(u128_low(ch))
              : 64 + (clh ? __builtin_clzll(clh)
                      : 64 + __builtin_clzll(u128_low(cl))));
    /* ex < 256 since |A| > |B| */

    /* If ex=0 or ex=1, the rounding error is bounded by 2 ulps. */
    if (ex > 0)
      {
        /* shift A by ex bits to the left, and B by ex-k bits to the left */
        if (ex >= 128)
          {
            ah = u128_lshift (al, ex - 128);
            al = u128_from_u64 (0);
          }
        else /* 1 <= ex < 128 */
          {
            ah = u128_or (u128_lshift (ah, ex), u128_rshift (al, 128 - ex));
            al = u128_lshift (al, ex);
          }
        int sh = ex - k;
        bh = b->rh;
        bl = b->rl;
        if (sh >= 0) {
          if (sh >= 128) {
            bh = u128_lshift (bl, sh - 128);
            bl = u128_from_u64 (0);
          }
          else if (sh > 0) { /* 1 <= sh < 128 */
            ah = u128_or (u128_lshift (bh, sh), u128_rshift (bl, 128 - sh));
            bl = u128_lshift (bl, sh);
          }
        }
        else { /* sh < 0: shift b by -sh bits to the right */
          int j = -sh;
          if (j >= 128) {
	    bl = u128_rshift (bh, j - 128);
            bh = u128_from_u64 (0);
          }
          else { /* 0 < j < 128 (j cannot be 0 since sh < 0) */
            bl = u128_or (u128_lshift (bh, 128 - j), u128_rshift (bl, j));
            bh = u128_rshift (bh, j);
          }
        }
        r->ex -= ex;
        ch = u128_sub (ah, bh);

        if (subu128 (al, bl, &cl))
          ch = u128_sub (ch, u128_from_u64 (1));
        /* we cannot have C=0 since |A| > |B| */
        chh = u128_high (ch);
        clh = u128_high (cl);
        ex =
          chh ? __builtin_clzll(chh)
          : 64 + (!u128_eq (ch, u128_from_u64(0)) ? __builtin_clzll(u128_low(ch))
                  : 64 + (clh ? __builtin_clzll(clh)
                          : 64 + __builtin_clzll(u128_low(cl))));
      }
    if (ex) {
      ch = u128_or (u128_lshift (ch, ex), u128_rshift (cl, 128 - ex));
      cl = u128_lshift (cl, ex);
    }
    r->ex -= ex;
    /* We distinguish three cases according to the first value of ex:
       If ex=0, the error is bounded by 1 ulp (ignored part of B).
       If ex=1, the error is bounded by 2 ulps (ignored part of B
         multiplied by 2).
       In the case ex>1, the error is bounded by 1 ulp (truncated part of B),
       which might be multiplied by 2 if the final value of ex is 1.
    */
  } else { // addition case
    char cy = addu128 (ah, bh, &ch);

    if (addu128 (al, bl, &cl))
      cy += !u128_eq (u128_add (ch, u128_from_u64 (1)), u128_from_u64 (0));

    /* 0 <= cy <= 1 */

    if (cy) { // carry in the 256-bit addition
      cl = u128_or (u128_lshift (ch, 127), u128_rshift (cl, 1));
      //ch = ((u128)1 << 127) | (ch >> 1);
      ch = u128_or (u128_lshift (u128_from_u64 (1), 127), u128_rshift (ch, 1));
      r->ex ++;
    }
    /* In the addition case, the rounding error is bounded by 1 ulp. */
  }

  r->sgn = sgn;
  r->rh = ch;
  r->rl = cl;
}

/* same as add_qint, but only considers the upper 2 limbs of a and b,
   with rounding error < 2 ulps(128) */
static inline void
add_qint_22 (qint64_t *r, const qint64_t *a, const qint64_t *b) {
  if (u128_eq (a->rh, u128_from_u64 (0))) {
    cp_qint (r, b); // exact
    return;
  }

  if (u128_eq (b->rh, u128_from_u64 (0))) {
    cp_qint (r, a); // exact
    return;
  }

  /* compare the absolute values of a and b */
  switch (cmp_qint_22 (a, b)) {
  case 0: /* |a| = |b| */
    if (a->sgn ^ b->sgn) { /* signs differ */
      cp_qint (r, &ZERO_Q); // exact
      return;
    }

    cp_qint (r, a);
    r->ex++; // exact
    return;

  case -1: /* |a| < |b| */
    add_qint_22 (r, b, a);
    return;
  }

  // From now on, |A| > |B|

  u128 ah = a->rh, bh = b->rh;

  int64_t m_ex = a->ex;
  uint64_t k = a->ex - b->ex;

  if (k > 0)
    //bh = (k >= 128) ? 0 : bh >> k;
    bh = (k >= 128) ? u128_from_u64 (0) : u128_rshift (bh, k);

  /* now we have to add ah + bh, with error <= 1 ulp
     corresponding to the ignored part of bh */

  unsigned char sgn = a->sgn;
  uint64_t ex;
  u128 ch;

  r->ex = m_ex;

  if (a->sgn ^ b->sgn) { // subtraction case
    /* a and b have different signs: C = A + (-B) */
    ch = u128_sub (ah, bh);

    /* we cannot have ch=0 since |A| > |B| */
    uint64_t chh = u128_high (ch);
    ex = chh ? __builtin_clzll(chh) : 64 + __builtin_clzll(u128_low (ch));

    /* ex < 128 since |A| > |B| */

    if (ex > 0)
      {
        /* shift A and B by ex bits to the left */
        ah = u128_lshift (ah, ex);
        /* for B, we have to shift by k bits to the right and ex to the left */
        if (ex >= k)
	  bh = u128_lshift (b->rh, ex -k); // since ex < 128, the shift is well defined
        else
          bh = u128_rshift (b->rh, k - ex);
        /* since k < 128 (otherwise bh=0 and ch=ah thus ex=0), this shift is
           also well defined */
        r->ex -= ex;
        ch = u128_sub (ah, bh);

        /* we cannot have C=0 since |A| > |B| */
        chh = u128_high (ch);
        ex = chh ? __builtin_clzll(chh) : 64 + __builtin_clzll(u128_low(ch));
        /* rounding error is bounded by 1 ulp(128) */
      }
    ch = u128_lshift (ch, ex);
    /* if ex=1, the rounding error is multiplied by 2, thus < 2 ulp(128) */
    r->ex -= ex;
  } else { // addition case
    char cy = addu128 (ah, bh, &ch);

    if (cy) { // carry in the 128-bit addition
      ch = u128_or (u128_lshift (u128_from_u64 (1), 127),
		    u128_rshift (ch, 1));
      r->ex ++;
    }
    /* In the addition case, the rounding error is bounded by 1 ulp(128) */
  }

  r->sgn = sgn;
  r->rh = ch;
  r->rl = u128_from_u64 (0);
}

// Multiply two dint64_t numbers, with error < 14 ulps
static inline void
mul_qint (qint64_t *r, const qint64_t *a, const qint64_t *b) {
  u128 r33 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->hh));

  u128 r32 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->hl));
  u128 r23 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->hh));

  u128 r31 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->lh));
  u128 r13 = u128_mul (u128_from_u64 (a->lh), u128_from_u64 (b->hh));
  u128 r22 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->hl));

  u128 r30 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->ll));
  u128 r03 = u128_mul (u128_from_u64 (a->ll), u128_from_u64 (b->hh));
  u128 r21 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->lh));
  u128 r12 = u128_mul (u128_from_u64 (a->lh), u128_from_u64 (b->hl));

  u128 t6, t5, t4, t3;
  u128 c5, c4;

  t3 = u128_add (u128_add (u128_rshift (r12, 64), u128_rshift (r21, 64)),
		 u128_add (u128_rshift (r03, 64), u128_rshift (r30, 64)));
  /* no overflow since each term is < 2^64, thus the sum < 2^66 */

  /* t3 is the sum of the terms of "degree" 3 divided by 2^64 */

  c4 = u128_from_u64 (addu128 (r22, t3, &t4));
  c4 = u128_add (c4, u128_from_u64 (addu128 (r13, t4, &t4)));
  c4 = u128_add (c4, u128_from_u64 (addu128 (r31, t4, &t4)));

  /* (c4:1,t4:128) is the sum of the terms of "degree" 3 and 4 */

  c5 = u128_from_u64 (addu128 (r23, u128_rshift (t4, 64), &t5));
  c5 = u128_add (c5, u128_from_u64 (addu128 (r32, t5, &t5)));

  /* (c5:1,t5:128,low(t4):64) is the sum of the terms of "degree" 3 to 5 */

  t6 = u128_add (u128_add (r33, u128_or (u128_lshift (c5, 64),
					 u128_rshift (t5, 64))),
		 c4);

  /* (t6:128,low(t5):64,low(t4):64) is the sum of the terms of "degree" 3-6 */

  /* No carry can happen since the full product of the significands is
     bounded by 2^512.
     The approximated sum is:
     t6 (128 bits) + low(t5) (64 bits) + low(t4) (64 bits) + low(t3) (64 bits)
     with error bounded by:
     * 3*(B-1)^2/B^2 ulp for the neglected terms of "degree" 2: r20 + r11 + r02
     * 2*(B-1)^2/B^3 ulp for the neglected terms of "degree" 1: r10 + r01
     * 1*(B-1)^2/B^4 ulp for the neglected term of "degree" 0: r00
     * 1 ulp for each of the neglected low parts of r12, r21, r03 and r30
       thus 4 ulps in total
     The sum of the first three terms is less than 3, thus bounded by 3 ulps.
     This yields an error bound of 7 ulps so far.
  */

  uint64_t ex = !(u128_low (u128_rshift (t6, 127)));

  t5 = u128_or (u128_lshift (t5, 64),
		u128_and (t4, u128_from_u64 (UINT64_C(0xffffffffffffffff))));
  if (ex) { /* ex=1 */
    r->rh = u128_or (u128_lshift (t6, 1), u128_rshift (t5, 127));
    r->rl = u128_lshift (t5, 1);
    /* the previous rounding error is multiplied by 2, thus < 14 ulps now */
  }
  else { /* ex=0 */
    r->rh = t6;
    r->rl = t5;
    /* error < 7 ulps */
  }

  r->ex = a->ex + b->ex + 1 - ex;

  r->sgn = a->sgn ^ b->sgn;
}

/* same as mul_qint, but considering only the upper 3 limbs from a and b,
   and with error < 6 ulps */
static inline void
mul_qint_33 (qint64_t *r, const qint64_t *a, const qint64_t *b) {
  u128 r33 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->hh));

  u128 r32 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->hl));
  u128 r23 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->hh));

  u128 r31 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->lh));
  u128 r13 = u128_mul (u128_from_u64 (a->lh), u128_from_u64 (b->hh));
  u128 r22 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->hl));

  u128 r21 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->lh));
  u128 r12 = u128_mul (u128_from_u64 (a->lh), u128_from_u64 (b->hl));

  u128 t6, t5, t4, t3;
  u128 c5, c4;

  t3 = u128_add (u128_rshift (r12, 64), u128_rshift (r21, 64));
  /* no overflow since each term is < 2^64, thus the sum < 2*2^64 */

  /* t3 is the sum of the terms of "degree" 3 divided by 2^64 */

  c4 = u128_from_u64 (addu128 (r22, t3, &t4));
  c4 = u128_add (c4, u128_from_u64 (addu128 (r13, t4, &t4)));
  c4 = u128_add (c4, u128_from_u64 (addu128 (r31, t4, &t4)));

  /* (c4:1,t4:128) is the sum of the terms of "degree" 3 and 4 */

  c5 = u128_from_u64 (addu128 (r23, u128_rshift (t4, 64), &t5));
  c5 = u128_add (c5, u128_from_u64 (addu128 (r32, t5, &t5)));

  /* (c5:1,t5:128,low(t4):64) is the sum of the terms of "degree" 3 to 5 */

  t6 = u128_add (u128_add (r33, u128_or (u128_lshift (c5, 64),
					 u128_rshift (t5, 64))),
		 c4);

  /* (t6:128,low(t5):64,low(t4):64) is the sum of the terms of "degree" 3-6 */

  /* No carry can happen since the full product of the significands is
     bounded by 2^512.
     The approximated sum is:
     t6 (128 bits) + low(t5) (64 bits) + low(t4) (64 bits) + low(t3) (64 bits)
     with error bounded by:
     * 1 ulp for the neglected term of "degree" 2: r11
     * 1 ulp for each of the neglected low parts of r12, r21
       thus 2 ulps in total
     This yields an error bound of 3 ulps so far.
  */

  uint64_t ex = !(u128_low (u128_rshift (t6, 127)));

  t5 = u128_or (u128_lshift (t5, 64),
		u128_and (t4, u128_from_u64 (UINT64_C(0xffffffffffffffff))));
  if (ex) { /* ex=1 */
    r->rh = u128_or (u128_lshift (t6, 1), u128_rshift (t5, 127));
    r->rl = u128_lshift (t5, 1);
    /* the previous rounding error is multiplied by 2, thus < 6 ulps now */
  }
  else { /* ex=0 */
    r->rh = t6;
    r->rl = t5;
    /* error < 3 ulps */
  }

  r->ex = a->ex + b->ex + 1 - ex;

  r->sgn = a->sgn ^ b->sgn;
}

/* same as mul_qint, but considering only the upper limb from b,
   and with error < 2 ulps */
static inline void
mul_qint_41 (qint64_t *r, const qint64_t *a, const qint64_t *b) {
  u128 r33 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->hh));

  u128 r23 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->hh));

  u128 r13 = u128_mul (u128_from_u64 (a->lh), u128_from_u64 (b->hh));

  u128 r03 = u128_mul (u128_from_u64 (a->ll), u128_from_u64 (b->hh));

  u128 t6, t5, t4, t3;
  u128 c5, c4;

  t3 = u128_rshift (r03, 64);

  /* t3 is the term of "degree" 3 divided by 2^64 */

  c4 = u128_from_u64 (addu128 (r13, t3, &t4));

  /* (c4:1,t4:128) is the sum of the terms of "degree" 3 and 4 */

  c5 = u128_from_u64 (addu128 (r23, u128_rshift (t4, 64), &t5));

  /* (c5:1,t5:128,low(t4):64) is the sum of the terms of "degree" 3 to 5 */

  t6 = u128_add (u128_add (r33, u128_or (u128_lshift (c5, 64),
					 u128_rshift (t5, 64))),
		 c4);

  /* (t6:128,low(t5):64,low(t4):64) is the sum of the terms of "degree" 3-6 */

  /* No carry can happen since the full product of the significands is
     bounded by 2^512.
     The approximated sum is:
     t6 (128 bits) + low(t5) (64 bits) + low(t4) (64 bits) + low(t3) (64 bits)
     with error bounded by 1 ulp for the neglected low part of r03.
  */

  uint64_t ex = !(u128_low (u128_rshift (t6, 127)));

  t5 = u128_or (u128_lshift (t5, 64),
		u128_and (t4, u128_from_u64 (UINT64_C(0xffffffffffffffff))));
  if (ex) { /* ex=1 */
    r->rh = u128_or (u128_lshift (t6, 1), u128_rshift (t5, 127));
    r->rl = u128_lshift (t5, 1);
    /* the previous rounding error is multiplied by 2, thus < 2 ulps now */
  }
  else { /* ex=0 */
    r->rh = t6;
    r->rl = t5;
    /* error < 1 ulp */
  }

  r->ex = a->ex + b->ex + 1 - ex;

  r->sgn = a->sgn ^ b->sgn;
}

/* same as mul_qint, but considering only the 3 upper limbs from a,
   and the upper limb from b, with no error (exact product) */
static inline void
mul_qint_31 (qint64_t *r, const qint64_t *a, const qint64_t *b) {
  u128 r33 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->hh));
  u128 r23 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->hh));
  u128 r13 = u128_mul (u128_from_u64 (a->lh), u128_from_u64 (b->hh));

  u128 t6, t5, t4;
  u128 c5;

  t4 = r13;

  /* t4 is the only term of "degree" 4 */

  c5 = u128_from_u64 (addu128 (r23, u128_rshift (t4, 64), &t5));

  /* (c5:1,t5:128,low(t4):64) is the sum of the terms of "degree" 4 to 5 */

  t6 = u128_add (r33, u128_or (u128_lshift (c5, 64),
			       u128_rshift (t5, 64)));

  /* (t6:128,low(t5):64,low(t4):64) is the sum of the terms of "degree" 4-6 */

  /* No carry can happen since the full product of the significands is
     bounded by 2^512.
     The approximated sum is:
     t6 (128 bits) + low(t5) (64 bits) + low(t4) (64 bits)
     with no error.
  */

  uint64_t ex = !(u128_low (u128_rshift (t6, 127)));

  t5 = u128_or (u128_lshift (t5, 64),
		u128_and (t4, u128_from_u64 (UINT64_C(0xffffffffffffffff))));
  if (ex) { /* ex=1 */
    r->rh = u128_or (u128_lshift (t6, 1), u128_rshift (t5, 127));
    r->rl = u128_lshift (t5, 1);
    /* the previous rounding error is multiplied by 2, thus < 2 ulps now */
  }
  else { /* ex=0 */
    r->rh = t6;
    r->rl = t5;
    /* error < 1 ulp */
  }

  r->ex = a->ex + b->ex + 1 - ex;

  r->sgn = a->sgn ^ b->sgn;
}

/* same as mul_qint, but considering only the 2 upper limbs from a and b,
   with no error (exact product) */
static inline void
mul_qint_22 (qint64_t *r, const qint64_t *a, const qint64_t *b) {
  u128 r33 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->hh));

  u128 r32 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->hl));
  u128 r23 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->hh));

  u128 r22 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->hl));

  u128 t6, t5, t4;
  u128 c5;

  t4 = r22;

  c5 = u128_from_u64 (addu128 (r23, u128_rshift (t4, 64), &t5));
  c5 = u128_add (c5, u128_from_u64 (addu128 (r32, t5, &t5)));

  /* (c5:1,t5:128,low(t4):64) is the sum of the terms of "degree" 3 to 5 */

  t6 = (u128_add (r33, u128_or (u128_lshift (c5, 64),
				u128_rshift (t5, 64))));

  /* (t6:128,low(t5):64,low(t4):64) is the sum of the terms of "degree" 3-6 */

  /* No carry can happen since the full product of the significands is
     bounded by 2^512.
     The exact sum is:
     t6 (128 bits) + low(t5) (64 bits) + low(t4) (64 bits)
  */

  uint64_t ex = !(u128_low (u128_rshift (t6, 127)));

  t5 = u128_or (u128_lshift (t5, 64),
		u128_and (t4, u128_from_u64 (UINT64_C(0xffffffffffffffff))));
  if (ex) { /* ex=1 */
    r->rh = u128_or (u128_lshift (t6, 1), u128_rshift (t5, 127));
    r->rl = u128_lshift (t5, 1);
  }
  else { /* ex=0 */
    r->rh = t6;
    r->rl = t5;
  }

  r->ex = a->ex + b->ex + 1 - ex;

  r->sgn = a->sgn ^ b->sgn;
}

/* same as mul_qint, but considering only the 2 upper limbs from a, and the
   upper limb of b, with no error (exact product) */
static inline void
mul_qint_21 (qint64_t *r, const qint64_t *a, const qint64_t *b) {
  u128 r33 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->hh));
  u128 r23 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (b->hh));

  u128 t6 = u128_add (r33, u128_lshift (r23, 64));

  /* No carry can happen since the full product of the significands is
     bounded by 2^512.
  */

  uint64_t ex = !(u128_low (u128_rshift (t6, 127)));

  u128 t5 = u128_lshift (r23, 64);
  if (ex) { /* ex=1 */
    r->rh = u128_or (u128_lshift (t6, 1), u128_rshift (t5, 127));
    r->rl = u128_lshift (t5, 1);
  }
  else { /* ex=0 */
    r->rh = t6;
    r->rl = t5;
  }

  r->ex = a->ex + b->ex + 1 - ex;

  r->sgn = a->sgn ^ b->sgn;
}

/* same as mul_qint, but considering only the upper limb from a and b,
   with no error (exact product) */
static inline void
mul_qint_11 (qint64_t *r, const qint64_t *a, const qint64_t *b) {
  u128 t6 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (b->hh));

  uint64_t ex = !(u128_low (u128_rshift (t6, 127)));

  /* ex can be 0 or 1 */

  r->rh = u128_lshift (t6, ex);
  r->rl = u128_from_u64 (0);
  r->ex = a->ex + b->ex + 1 - ex;
  r->sgn = a->sgn ^ b->sgn;
}

// Multiply an integer with a qint64_t variable, with error < 2 ulps
static inline void mul_qint_2 (qint64_t *r, int64_t b, const qint64_t *a) {
  if (!b) {
    cp_qint (r, &ZERO_Q); // exact
    return;
  }

  uint64_t c = b < 0 ? -b : b;
  if (c == 1) {
    cp_qint (r, a); // exact
    r->sgn = (b < 0) ^ a->sgn;

    return;
  }

  r->sgn = (b < 0) ^ a->sgn;
  r->ex = a->ex + 64;

  /* scale c so that 2^63 <= c < 2^64 */
  int k = __builtin_clzll (c);
  c = c << k;
  r->ex -= k;

  u128 t3 = u128_mul (u128_from_u64 (a->hh), u128_from_u64 (c));
  u128 t2 = u128_mul (u128_from_u64 (a->hl), u128_from_u64 (c));
  u128 t1 = u128_mul (u128_from_u64 (a->lh), u128_from_u64 (c));
  u128 t0 = u128_mul (u128_from_u64 (a->ll), u128_from_u64 (c));

  u128 cy;
  u128 t = u128_rshift (t0, 64);

  /* t:64 is the term of degree 0 (divided by 2^64) */

  cy = u128_from_u64 (addu128 (t, t1, &t1));
  /* (cy:1,t1:128) is the sum of the terms of degree 0 and 1 */

  t = u128_or (u128_lshift (cy, 64), u128_rshift (t1, 64));
  cy = u128_from_u64 (addu128 (t, t2, &t2));
  /* (cy:1,t2:128,low(t1):64) is the sum of the terms of degree 0 to 2 */

  t3 = u128_add (t3, u128_or (u128_lshift (cy, 64), u128_rshift (t2, 64)));
  /* (t3,low(t2):64,low(t1):64) is the sum of the terms of degree 0 to 3 */

  uint32_t ex = __builtin_clzll (u128_high (t3));

  t2 = u128_or (u128_lshift (t2, 64),
		u128_and (t1, u128_from_u64 (UINT64_C(0xffffffffffffffff))));
  /* ex is 0 or 1 because a and c are normalized (2^63 <= a->hh, c < 2^64) */

  /* we only ignore the low part of t0 which contributes less than 1 ulp */

  if (ex)
    {
      r->rh = u128_or (u128_lshift (t3, 1), u128_rshift (t2, 127));
      r->rl = u128_lshift (t2, 1);
      /* the error is scaled by 2, thus less than 2 ulps */
      r->ex --;
    }
  else
    {
      r->rh = t3;
      r->rl = t2;
      /* error less than 1 ulp in that case */
    }
}

#endif
