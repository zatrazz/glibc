/* Correctly-rounded tangent function for binary64 value.

Copyright (c) 2022-2025 Paul Zimmermann and Tom Hubrecht

The original version of this file was copied from the CORE-MATH
project (file src/binary64/tan/tan.c, revision 8ea8ea35).

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

#include <stdint.h>
#include <fenv.h>
#include <math.h>
#include <libm-alias-double.h>
#include <math_uint128.h>
#include "math_config.h"

#ifndef SECTION
#  define SECTION
#endif

#include "dint64.h"

/* for 0 <= i < 256, Tinv[i] = floor(2^127/(2^63+i*2^55+2^55-1)) */
static uint64_t Tinv[256]
    = { 0xff00ff00ff00ff02, 0xfe03f80fe03f80ff, 0xfd08e5500fd08e56,
	0xfc0fc0fc0fc0fc11, 0xfb18856506ddaba7, 0xfa232cf252138ac1,
	0xf92fb2211855a867, 0xf83e0f83e0f83e11, 0xf74e3fc22c700f76,
	0xf6603d980f6603db, 0xf57403d5d00f5742, 0xf4898d5f85bb3952,
	0xf3a0d52cba872338, 0xf2b9d6480f2b9d66, 0xf1d48bcee0d399fc,
	0xf0f0f0f0f0f0f0f2, 0xf00f00f00f00f010, 0xef2eb71fc4345239,
	0xee500ee500ee5010, 0xed7303b5cc0ed731, 0xec979118f3fc4da3,
	0xebbdb2a5c1619c8d, 0xeae56403ab959010, 0xea0ea0ea0ea0ea10,
	0xe939651fe2d8d35d, 0xe865ac7b7603a198, 0xe79372e225fe30da,
	0xe6c2b4481cd8568a, 0xe5f36cb00e5f36cc, 0xe525982af70c880f,
	0xe45932d7dc52100f, 0xe38e38e38e38e38f, 0xe2c4a6886a4c2e11,
	0xe1fc780e1fc780e3, 0xe135a9c97500e137, 0xe070381c0e070383,
	0xdfac1f74346c5760, 0xdee95c4ca037ba58, 0xde27eb2c41f3d9d2,
	0xdd67c8a60dd67c8b, 0xdca8f158c7f91ab9, 0xdbeb61eed19c5959,
	0xdb2f171df770291a, 0xda740da740da740f, 0xd9ba4256c0366e92,
	0xd901b2036406c80f, 0xd84a598ec9151f44, 0xd79435e50d79435f,
	0xd6df43fca482f00e, 0xd62b80d62b80d62c, 0xd578e97c3f5fe552,
	0xd4c77b03531dec0e, 0xd4173289870ac52e, 0xd3680d3680d3680e,
	0xd2ba083b445250ac, 0xd20d20d20d20d20e, 0xd161543e28e50275,
	0xd0b69fcbd2580d0c, 0xd00d00d00d00d00e, 0xcf6474a8819ec8ea,
	0xcebcf8bb5b4169cc, 0xce168a7725080ce2, 0xcd712752a886d243,
	0xccccccccccccccce, 0xcc29786c7607f99f, 0xcb8727c065c393e1,
	0xcae5d85f1bbd6c96, 0xca4587e6b74f032a, 0xc9a633fcd967300d,
	0xc907da4e871146ad, 0xc86a78900c86a78a, 0xc7ce0c7ce0c7ce0d,
	0xc73293d789b9f839, 0xc6980c6980c6980d, 0xc5fe740317f9d00d,
	0xc565c87b5f9d4d1c, 0xc4ce07b00c4ce07c, 0xc4372f855d824ca6,
	0xc3a13de60495c774, 0xc30c30c30c30c30d, 0xc2780613c0309e02,
	0xc1e4bbd595f6e948, 0xc152500c152500c2, 0xc0c0c0c0c0c0c0c1,
	0xc0300c0300c0300d, 0xbfa02fe80bfa02ff, 0xbf112a8ad278e8de,
	0xbe82fa0be82fa0bf, 0xbdf59c91700bdf5a, 0xbd69104707661aa3,
	0xbcdd535db1cc5b7c, 0xbc52640bc52640bd, 0xbbc8408cd63069a1,
	0xbb3ee721a54d880c, 0xbab656100bab6562, 0xba2e8ba2e8ba2e8c,
	0xb9a7862a0ff46588, 0xb92143fa36f5e02f, 0xb89bc36ce3e0453b,
	0xb81702e05c0b8171, 0xb79300b79300b794, 0xb70fbb5a19be3659,
	0xb68d31340e4307d9, 0xb60b60b60b60b60c, 0xb58a485518d1e7e4,
	0xb509e68a9b948220, 0xb48a39d44685fe97, 0xb40b40b40b40b40c,
	0xb38cf9b00b38cf9b, 0xb30f63528917c80c, 0xb2927c29da5519d0,
	0xb21642c8590b2165, 0xb19ab5c45606f00c, 0xb11fd3b80b11fd3c,
	0xb0a59b418d749d54, 0xb02c0b02c0b02c0b, 0xafb321a1496fdf0f,
	0xaf3addc680af3ade, 0xaec33e1f671529a5, 0xae4c415c9882b931,
	0xadd5e6323fd48a87, 0xad602b580ad602b6, 0xaceb0f891e6551bc,
	0xac7691840ac76919, 0xac02b00ac02b00ac, 0xab8f69e28359cd12,
	0xab1cbdd3e2970f60, 0xaaaaaaaaaaaaaaab, 0xaa392f35dc17f00b,
	0xa9c84a47a07f5638, 0xa957fab5402a55ff, 0xa8e83f5717c0a8e9,
	0xa87917088e262b70, 0xa80a80a80a80a80b, 0xa79c7b16ea64d422,
	0xa72f05397829cbc2, 0xa6c21df6e1625c80, 0xa655c4392d7b73a8,
	0xa5e9f6ed347f0721, 0xa57eb50295fad40b, 0xa513fd6bb00a5140,
	0xa4a9cf1d96833751, 0xa44029100a440291, 0xa3d70a3d70a3d70b,
	0xa36e71a2cb033129, 0xa3065e3fae7cd0e0, 0xa29ecf163bb6500a,
	0xa237c32b16cfd772, 0xa1d139855f7268ee, 0xa16b312ea8fc377d,
	0xa105a932f2ca891f, 0xa0a0a0a0a0a0a0a1, 0xa03c1688732b3032,
	0x9fd809fd809fd80a, 0x9f747a152d7836d0, 0x9f1165e7254813e2,
	0x9eaecc8d53ae2ddf, 0x9e4cad23dd5f3a20, 0x9deb06c9194aa416,
	0x9d89d89d89d89d8a, 0x9d2921c3d6411308, 0x9cc8e160c3fb19b9,
	0x9c69169b30446dfa, 0x9c09c09c09c09c0a, 0x9baade8e4a2f6e10,
	0x9b4c6f9ef03a3caa, 0x9aee72fcf957c10f, 0x9a90e7d95bc609a9,
	0x9a33cd67009a33ce, 0x99d722dabde58f06, 0x997ae76b50efd00a,
	0x991f1a515885fb37, 0x98c3bac74f5db00a, 0x9868c809868c8099,
	0x980e4156201301c8, 0x97b425ed097b425f, 0x975a750ff68a58af,
	0x97012e025c04b80a, 0x96a850096a850097, 0x964fda6c0964fda7,
	0x95f7cc72d1b887e9, 0x95a02568095a0257, 0x9548e4979e0829fd,
	0x94f2094f2094f209, 0x949b92ddc02526e5, 0x9445809445809446,
	0x93efd1c50e726b7c, 0x939a85c40939a85c, 0x93459be6b009345a,
	0x92f113840497889c, 0x929cebf48bbd90e5, 0x9249249249249249,
	0x91f5bcb8bb02d9cd, 0x91a2b3c4d5e6f809, 0x9150091500915009,
	0x90fdbc090fdbc091, 0x90abcc0242af3009, 0x905a38633e06c43b,
	0x9009009009009009, 0x8fb823ee08fb823f, 0x8f67a1e3fdc26179,
	0x8f1779d9fdc3a219, 0x8ec7ab397255e41d, 0x8e78356d1408e783,
	0x8e2917e0e702c6ce, 0x8dda520237694809, 0x8d8be33f95d71590,
	0x8d3dcb08d3dcb08d, 0x8cf008cf008cf009, 0x8ca29c046514e023,
	0x8c55841c815ed5ca, 0x8c08c08c08c08c09, 0x8bbc50c8deb420c0,
	0x8b70344a139bc75b, 0x8b246a87e19008b2, 0x8ad8f2fba9386823,
	0x8a8dcd1feeae465c, 0x8a42f8705669db46, 0x89f87469a23920e0,
	0x89ae4089ae4089ae, 0x89645c4f6e055dec, 0x891ac73ae9819b50,
	0x88d180cd3a4133d7, 0x8888888888888889, 0x883fddf00883fddf,
	0x87f78087f78087f8, 0x87af6fd5992d0d40, 0x8767ab5f34e47ef1,
	0x872032ac13008720, 0x86d905447a34acc6, 0x869222b1acf1ce96,
	0x864b8a7de6d1d608, 0x86053c345a0b8473, 0x85bf37612cee3c9b,
	0x85797b917765ab89, 0x8534085340853408, 0x84eedd357c1b0085,
	0x84a9f9c8084a9f9d, 0x84655d9bab2f1008, 0x8421084210842108,
	0x83dcf94dc7570ce1, 0x839930523fbe3368, 0x8355ace3c897db10,
	0x83126e978d4fdf3b, 0x82cf750393ac3319, 0x828cbfbeb9a020a3,
	0x824a4e60b3262bc5, 0x8208208208208208, 0x81c635bc123fdf8e,
	0x81848da8faf0d277, 0x814327e3b94f462f, 0x8102040810204081,
	0x80c121b28bd1ba98, 0x8080808080808081, 0x8040201008040201,
	0x8000000000000000 };

/* put in r an approximation of 1/a, assuming a is not zero,
   with error bounded by 4.003 ulps (relative error < 2^-124.999) */
static inline void
inv_dint (dint64_t *r, dint64_t *a)
{
  uint64_t h = a->hi; /* 2^63 <= h < 2^64 */
  /* First compute a 64-bit inverse t of h, such that
     t*h ~ 2^127, see routine inv_dint() in tan.sage.
     We note a = h/2^63, then 1 <= a < 2, and we write x = t/2^64 for
     the approximation of 1/a, with 1/2 <= x < 1. */
  int i = (h >> 55) & 0xff;
  uint64_t t = Tinv[i];
  /* now t is accurate to about 8 bits, more precisely the integer residual
     2^127 - h*t is bounded by 662027951208051476078044039717322752 < 2^118.995
     (attained for i=0 and h=2^63). The integer residual 2^127 - t*h
     equals 2^127*(1-a*x), thus 0 <= 1-ax < 2^-8.005. */

  /* first Newton iteration */
  u128 e = u128_sub (u128_lshift (u128_from_u64 (1), 127),
		     u128_mul (u128_from_u64 (h), u128_from_u64 (t))); // exact
  /* as Tinv was computed, we have 0 < e < 2^119 */
  e = u128_mul (u128_from_u64 (t), u128_rshift (e, 55));
  t += u128_low (u128_rshift (e, 72));
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
  e = u128_sub (
      u128_lshift (u128_from_u64 (1), 127),
      u128_mul (u128_from_u64 (h), u128_from_u64 (t))); // 0 <= e < 2^111
  e = u128_mul (u128_from_u64 (t), u128_rshift (e, 47));
  t += u128_low (u128_rshift (e, 80));
  /* With the same reasoning as above, the truncation e >> 47 induces an
     error of at most 2^-16 on t, and the truncation e >> 80 an error of
     at most 1, giving a total of 1+2^-16.
     Since h < 2^64, this can increase by at most 2^64*(1+2^-16) the value
     of 2^127 - h*t:
     2^127 - h*t < 2^(2*110.991)/2^127 + 2^64*(1+2^-16) < 2^94.9821. */

  /* third Newton iteration */
  e = u128_sub (
      u128_lshift (u128_from_u64 (1), 127),
      u128_mul (u128_from_u64 (h), u128_from_u64 (t))); // 0 <= e < 2^95
  e = u128_mul (u128_from_u64 (t), u128_rshift (e, 31));
  t += u128_low (u128_rshift (e, 96));
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
  mul_dint_21 (q, a, r); /* -a*r, error <= 2 ulps */
  r->sgn = 0;		 /* restore sign */
  add_dint (q, &ONE, q); /* 1-a*r, error <= 2 ulps */
  mul_dint (q, r, q);	 /* r*(1-a*r), error <= 6 ulps */
  add_dint (r, r, q);	 /* error <= 2 ulps */
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
}

/* put in r an approximation of b/a, assuming a is not zero,
   with relative error < 2^-123.67 */
static inline void
div_dint (dint64_t *r, dint64_t *b, dint64_t *a)
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

#include "dint64_data.h"

// Multiply exactly a and b, such that *hi + *lo = a * b.
static inline void
a_mul (double *hi, double *lo, double a, double b)
{
  *hi = a * b;
  *lo = fma (a, b, -*hi);
}

/* Multiply a double with a double double : a * (bh + bl)
   with error bounded by ulp(lo) */
static inline void
s_mul (double *hi, double *lo, double a, double bh, double bl)
{
  a_mul (hi, lo, a, bh); /* exact */
  *lo = fma (a, bl, *lo);
  /* the error is bounded by ulp(lo), where |lo| < |a*bl| + ulp(hi) */
}

// Returns (ah + al) * (bh + bl) - (al * bl)
// We can ignore al * bl when assuming al <= ulp(ah) and bl <= ulp(bh)
static inline void
d_mul (double *hi, double *lo, double ah, double al, double bh, double bl)
{
  double s, t;

  a_mul (hi, &s, ah, bh);
  t = fma (al, bh, s);
  *lo = fma (ah, bl, t);
}

static inline void
fast_two_sum (double *hi, double *lo, double a, double b)
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
  *h = PSfast[4];		// degree 7
  *h = fma (*h, uh, PSfast[3]); // degree 5
  *h = fma (*h, uh, PSfast[2]); // degree 3
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
  *h = PCfast[4];		// degree 6
  *h = fma (*h, uh, PCfast[3]); // degree 4
  *h = fma (*h, uh, PCfast[2]); // degree 2
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
  mul_dint_21 (Y, X2, PS + 5); // degree 11
  add_dint (Y, Y, PS + 4);     // degree 9
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PS + 3); // degree 7
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PS + 2); // degree 5
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PS + 1); // degree 3
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PS + 0); // degree 1
  mul_dint (Y, Y, X);	   // multiply by X
}

/* Put in Y an approximation of cos2pi(X), for 0 <= X < 2^-11,
   where X2 approximates X^2.
   Absolute/relative error bounded by 2^-125.999 with 0.999995 < Y <= 1
   (see evalPC() in sin.sage). */
static void
evalPC (dint64_t *Y, dint64_t *X2)
{
  mul_dint_21 (Y, X2, PC + 5); // degree 10
  add_dint (Y, Y, PC + 4);     // degree 8
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PC + 3); // degree 6
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PC + 2); // degree 4
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PC + 1); // degree 2
  mul_dint (Y, Y, X2);
  add_dint (Y, Y, PC + 0); // degree 0
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
      X->hi = u128_high (u) + (X->lo < u128_low (u));
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
  u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[i + 3])); // i+3 <= 18
  c[0] = u128_low (u);
  c[1] = u128_high (u);
  u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[i + 2]));
  c[1] += u128_low (u);
  c[2] = u128_high (u) + (c[1] < u128_low (u));
  u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[i + 1]));
  c[2] += u128_low (u);
  c[3] = u128_high (u) + (c[2] < u128_low (u));
  u = u128_mul (u128_from_u64 (X->hi), u128_from_u64 (T[i]));
  c[3] += u128_low (u);
  c[4] = u128_high (u) + (c[3] < u128_low (u));

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
      u = u128_mul (u128_from_u64 (X->hi),
		    u128_from_u64 (T[i + 4])); // i+4 <= 19
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
  if (c1)
    {
      e = __builtin_clzll (c1);
      if (e)
	{
	  c1 = (c1 << e) | (c0 >> (64 - e));
	  c0 = c0 << e;
	}
      f = 0x3fe - e;
      *h = asdouble ((f << 52) | ((c1 << 1) >> 12));
      c0 = (c1 << 53) | (c0 >> 11);
      if (c0)
	{
	  g = __builtin_clzll (c0);
	  if (g)
	    c0 = c0 << g;
	  *l = asdouble (((f - 53 - g) << 52) | ((c0 << 1) >> 12));
	}
      else
	*l = 0;
    }
  else if (c0)
    {
      e = __builtin_clzll (c0);
      f = 0x3fe - 64 - e;
      c0 = c0 << (e + 1); // most significant bit shifted out
      /* put the upper 52 bits of c0 into h */
      *h = asdouble ((f << 52) | (c0 >> 12));
      /* put the lower 12 bits of c0 into l */
      c0 = c0 << 52;
      if (c0)
	{
	  g = __builtin_clzll (c0);
	  c0 = c0 << (g + 1);
	  *l = asdouble (((f - 64 - g) << 52) | (c0 >> 12));
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
  if (__glibc_likely (x <= 0x1.921fb54442d17p+2)) // x < 2*pi
    {
      /* | CH+CL - 1/(2pi) | < 2^-110.523 */
#define CH 0x1.45f306dc9c883p-3
#define CL -0x1.6b01ec5417056p-57
      a_mul (h, l, CH, x); // exact
      *l = fma (CL, x, *l);
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
      uint64_t t = asuint64 (x);
      int e = (t >> 52) & 0x7ff; /* 1025 <= e <= 2046 */
      /* We have 2^(e-1023) <= x < 2^(e-1022), thus
	 ulp(x) is a multiple of 2^(e-1075), for example
	 if x is just above 2*pi, e=1025, 2^2 <= x < 2^3,
	 and ulp(x) is a multiple of 2^-50.
	 On the other side 1/(2pi) ~ T[0]/2^64 + T[1]/2^128 + T[2]/2^192 + ...
	 Let i be the smallest integer such that 2^(e-1075)/2^(64*(i+1))
	 is not an integer, i.e., e - 1139 - 64i < 0, i.e.,
	 i >= (e-1138)/64. */
      uint64_t m = (1ull << 52) | (t & 0xfffffffffffffull);
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
	  v = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i + 3]));
	  u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i + 2]));
	  c[0] = u128_low (u128_add (u, u128_rshift (v, 64)));
	  /* There can be no overflow in (u >> 64) + (c[0] < u) since
	     u <= (2^64-1)*T[i+2] thus (u >> 64) < T[i+2], and T[i+2]+1
	     does not overflow. */
	  c[1] = u128_high (u) + (c[0] < u128_low (u));
	  u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i + 1]));
	  c[1] += u128_low (u);
	  c[2] = u128_high (u) + (c[1] < u128_low (u));
	  u = u128_mul (u128_from_u64 (m), u128_from_u64 (T[i]));
	  c[2] += u128_low (u);
	  e = 1139 + (i << 6) - e; // 1 <= e <= 64
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

  double i = floor (*h * 0x1p11);
  *h = fma (i, -0x1p-11, *h);
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

  double eh = fma (ah, -*h, bh);
  /* from the analysis above, we have |eh| < 2^-48.999 thus the rounding error
     is bounded by ulp(2^-48.999) = 2^-101 */
  double el = fma (al, -*h, bl);
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
  if (__glibc_unlikely (i == 0 && *h < 0x1p-37))
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
  ul = fma (*h + *h, *l, ul);
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

  static const double sgn[2] = { 1.0, -1.0 };
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
      X->sgn = 1;	       // negate X
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
  mul_dint (X2, X, X); // X2 approximates X^2
  evalPC (U, X2);      // cos2pi(X)
  /* since 0 <= X < 2^-11, we have 0.999 < U <= 1 */
  evalPS (V, X, X2); // sin2pi(X)
  /* since 0 <= X < 2^-11, we have 0 <= V < 0.0005 */

  // sin2pi(R) ~ sin2pi(i/2^11)*cos2pi(X)+cos2pi(i/2^11)*sin2pi(X)
  dint64_t Sin[1], UU[1], VV[1];
  mul_dint (UU, S + i, U);
  /* since 0 <= S[i] < 0.705 and 0.999 < Uin <= 1, we have
     0 <= U < 0.705 */
  mul_dint (VV, C + i, V);
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
  mul_dint (U, C + i, U);
  mul_dint (V, S + i, V);
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
	{ 0x1.dffffffffff1fp-22, 0x1.e000000000151p-22,
	  0x1.fffffffffffffp-76 },
	/* the following has 72 identical bits after the round bit */
	{ 0x1.dfffffffffc7cp-21, 0x1.e000000000546p-21,
	  -0x1.658bcedb6e1d4p-147 },
      };
      for (int j = 0; j < 2; j++)
	{
	  if (fabs (x) == exceptions[j][0])
	    return (x > 0) ? exceptions[j][1] + exceptions[j][2]
			   : -exceptions[j][1] - exceptions[j][2];
	}
      /* if we go here, we have a hard-to-round case, but since all
	 hard-to-round cases are known and pass all tests, we are ok */
    }

  if (neg)
    U->sgn = 1 - U->sgn;

  double y = dint_tod (U);

  return y;
}

double SECTION
__tan (double x)
{
  uint64_t t = asuint64 (x);
  int e = (t >> 52) & 0x7ff;

  if (__glibc_unlikely (e == 0x7ff)) /* NaN, +Inf and -Inf. */
    {
      if ((t << 1) == UINT64_C (0x7ff) << 53) // +/-Inf
	return __math_invalid (x);
      return x + x; // NaN
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
  uint64_t ux = t & 0x7fffffffffffffff;
  // 0x3e4d12ed0af1a27e = 0x1.d12ed0af1a27ep-27
  if (ux <= 0x3e4d12ed0af1a27e)
    {
      if (x == 0)
	return x;
      // Taylor expansion of tan(x) is x + x^3/3 around zero
      /* We have underflow when 0 < |x| < 2^-1022 or when |x| = 2^-1022
	 and rounding towards zero. */
      double res = fma (x, 0x1p-54, x);
      return __math_check_uflow_lt (res, 0x1p-1022);
    }

  double h, l, err;
  err = tan_fast (&h, &l, x);
  double left = h + (l - err), right = h + (l + err);
  if (__glibc_likely (left == right))
    return left;

  return tan_accurate (x);
}

#ifndef __tan
libm_alias_double (__tan, tan)
#endif
