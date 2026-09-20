/* Common code for the binary64 exp2m1 and exp10m1 implementations.

Copyright (c) 2022-2025 Alexei Sibidanov, Paul Zimmermann, Tom Hubrecht and
Claude-Pierre Jeannerod.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/exp2m1/exp2m1.c, revision 8ea8ea35).

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

#ifndef _EXP2M1_COMMON_H
#define _EXP2M1_COMMON_H

#include <math.h>
#include <stdint.h>
#include "math_config.h"

/* For 0 <= i < 64, T1[i] = (h,l) such that h+l is the best double-double
   approximation of 2^(i/64), and T2[i] likewise for 2^(i/2^12).  The
   approximation error is bounded by 2^-107.  */
extern const double __exp2m1_t1[64][2] attribute_hidden;
extern const double __exp2m1_t2[64][2] attribute_hidden;

/* Put in *HI + *LO a double-double approximation of exp(XH + XL), for the
   fast path (relative error bounded by 2^-74.139) and for the accurate
   path (bounded by 2^-113.218) respectively.  */
extern void __exp2m1_exp_1 (double *__hi, double *__lo, double __xh,
			    double __xl) attribute_hidden;
extern void __exp2m1_q_2 (double *__hi, double *__lo, double __zh,
			  double __zl) attribute_hidden;

static inline void
a_mul (double *hi, double *lo, double a, double b)
{
  *hi = a * b;
  *lo = fma (a, b, -*hi);
}

// Multiply a double with a double double : a * (bh + bl)
static inline void
s_mul (double *hi, double *lo, double a, double bh, double bl)
{
  a_mul (hi, lo, a, bh); /* exact */
  *lo = fma (a, bl, *lo);
}

// Returns (ah + al) * (bh + bl) - (al * bl)
static inline void
d_mul (double *hi, double *lo, double ah, double al, double bh, double bl)
{
  a_mul (hi, lo, ah, bh);
  *lo = fma (ah, bl, *lo);
  *lo = fma (al, bh, *lo);
}

// Add a + b, assuming |a| >= |b|
static inline void
fast_two_sum (double *hi, double *lo, double a, double b)
{
  double e;

  *hi = a + b;
  e = *hi - a; /* exact */
  *lo = b - e; /* exact */
}

// Add a + (bh + bl), assuming |a| >= |bh|
static inline void
fast_sum (double *hi, double *lo, double a, double bh, double bl)
{
  fast_two_sum (hi, lo, a, bh);
  /* |(a+bh)-(hi+lo)| <= 2^-105 |hi| and |lo| < ulp(hi) */
  *lo += bl;
  /* |(a+bh+bl)-(hi+lo)| <= 2^-105 |hi| + ulp(lo),
     where |lo| <= ulp(hi) + |bl|. */
}

#endif
