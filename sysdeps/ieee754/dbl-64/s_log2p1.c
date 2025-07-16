/* Correctly rounded log2(1+x) for binary64 values.

Copyright (c) 2022-2025 INRIA and CERN.
Authors: Paul Zimmermann and Tom Hubrecht.

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

#include <errno.h>
#include <math.h>
#include <libm-alias-double.h>
#include "math_config.h"
#include "dint.h"
#include "e_log_data.h"
#define CORE_MATH_SUPPORT_ERRNO


typedef union { double f; uint64_t u; } d64u64;

/* Add a + b, such that *hi + *lo approximates a + b.
   Assumes |a| >= |b|.  */
static void
fast_two_sum (double *hi, double *lo, double a, double b)
{
  double e;

  *hi = a + b;
  e = *hi - a; /* exact */
  *lo = b - e; /* exact */
  /* Now hi + lo = a + b exactly for rounding to nearest.
     For directed rounding modes, this is not always true.
     Take for example a = 1, b = 2^-200, and rounding up,
     then hi = 1 + 2^-52, e = 2^-52 (it can be proven that
     e is always exact), and lo = -2^52 + 2^-105, thus
     hi + lo = 1 + 2^-105 <> a + b = 1 + 2^-200.
     A bound on the error is given
     in "Note on FastTwoSum with Directed Roundings"
     by Paul Zimmermann, https://hal.inria.fr/hal-03798376, 2022.
     Theorem 1 says that
     the difference between a+b and hi+lo is bounded by 2u^2|a+b|
     and also by 2u^2|hi|. Here u=2^-53, thus we get:
     |(a+b)-(hi+lo)| <= 2^-105 min(|a+b|,|hi|) */
}

// Multiply exactly a and b, such that *hi + *lo = a * b.
static inline void a_mul(double *hi, double *lo, double a, double b) {
  *hi = a * b;
  *lo = __builtin_fma(a, b, -*hi);
}

// Multiply a double with a double double : a * (bh + bl)
static inline void s_mul (double *hi, double *lo, double a, double bh,
                          double bl) {
  double s;

  a_mul (hi, &s, a, bh); /* exact */
  *lo = __builtin_fma (a, bl, s);
}

// Returns (ah + al) * (bh + bl) - (al * bl)
static inline void d_mul(double *hi, double *lo, double ah, double al,
                         double bh, double bl) {
  a_mul (hi, lo, ah, bh);
  *lo = __builtin_fma (ah, bl, *lo);
  *lo = __builtin_fma (al, bh, *lo);
}

/* put in h+l a double-double approximation of log(z)-z for
   |z| < 0.00212097167968735, with absolute error bounded by 2^-78.25
   (see analyze_p1(-0.00212097167968735,0.00212097167968735)
   from accompanying file log1p.sage, which also yields |l| < 2^-69.99) */
static void
p_1 (double *h, double *l, double z)
{
  double z2h, z2l;
  a_mul (&z2h, &z2l, z, z); /* exact: z2h+z2l = z*z */
  double p56 = __builtin_fma (P_LOG1_X[6], z, P_LOG1_X[5]);
  double p34 = __builtin_fma (P_LOG1_X[4], z, P_LOG1_X[3]);
  double ph = __builtin_fma (p56, z2h, p34);
  /* ph approximates P[3]+P[4]*z+P[5]*z^2+P[6]*z^3 */
  ph = __builtin_fma (ph, z, P_LOG1_X[2]);
  /* ph approximates P[2]+P[3]*z+P[4]*z^2+P[5]*z^3+P[6]*z^4 */
  ph *= z2h;
  /* ph approximates P[2]*z^2+P[3]*z^3+P[4]*z^4+P[5]*z^5+P[6]*z^6 */
  fast_two_sum (h, l, -0.5 * z2h, ph * z);
  *l += -0.5 * z2l;
}

/* put in h+l a double-double approximation of log(z)-z for
   |z| < 0.03125, with absolute error bounded by 2^-67.14
   (see analyze_p1a(-0.03125,0.03125) from log1p.sage) */
static void
p_1a (double *h, double *l, double z)
{
  double z2h, z2l;
  if (__builtin_expect (__builtin_fabs (z) >= 0x1p-255, 1))
    a_mul (&z2h, &z2l, z, z);
  else // avoid spurious underflow
    z2h = z2l = 0;
  double z4h = z2h * z2h;
  double p910 = __builtin_fma (Pa[10], z, Pa[9]);
  double p78 = __builtin_fma (Pa[8], z, Pa[7]);
  double p56 = __builtin_fma (Pa[6], z, Pa[5]);
  double p34 = __builtin_fma (Pa[4], z, Pa[3]);
  double p710 = __builtin_fma (p910, z2h, p78);
  double p36 = __builtin_fma (p56, z2h, p34);
  double ph = __builtin_fma (p710, z4h, p36);
  ph = __builtin_fma (ph, z, Pa[2]);
  ph *= z2h;
  fast_two_sum (h, l, -0.5 * z2h, ph * z);
  *l += -0.5 * z2l;
}

/* Given 1 <= x < 2, where x = v.f, put in h+l a double-double approximation
   of log(2^e*x), with absolute error bounded by 2^-69.99 (details below).
*/
static void
cr_log_fast (double *h, double *l, int e, d64u64 v)
{
  uint64_t m = 0x10000000000000 + (v.u & 0xfffffffffffff);
  /* x = m/2^52 */
  /* if x > sqrt(2), we divide it by 2 to avoid cancellation */
  int c = m >= 0x16a09e667f3bcd;
  e += c; /* now -1074 <= e <= 1024 */
  static const double cy[] = {1.0, 0.5};
  static const uint64_t cm[] = {43, 44};

  int i = m >> cm[c];
  double y = v.f * cy[c];
#define OFFSET 362
  double r = _INVERSE[i-OFFSET];
  double l1 = _LOG_INV[i-OFFSET][0];
  double l2 = _LOG_INV[i-OFFSET][1];
  double z = __builtin_fma (r, y, -1.0); /* exact */
  /* evaluate P(z), for |z| < 0.00212097167968735 */
  double ph, pl;
  p_1 (&ph, &pl, z);

  /* Add e*log(2) to (h,l), where -1074 <= e <= 1023, thus e has at most
     11 bits. log2_h is an integer multiple of 2^-42, so that e*log2_h
     is exact. */
  static const double log2_h = 0x1.62e42fefa38p-1,
    log2_l = 0x1.ef35793c7673p-45;
  /* |log(2) - (h+l)| < 2^-102.01 */
  /* let hh = e * log2_h: hh is an integer multiple of 2^-42,
     with |hh| <= 1074*log2_h
     = 3274082061039582*2^-42. l1 is also an integer multiple of 2^-42,
     with |l1| <= 1524716581803*2^-42. Thus hh+l1 is an integer multiple of
     2^-42, with 2^42*|hh+l1| <= 3275606777621385 < 2^52, thus hh+l1 is exactly
     representable. */

  double ee = e;
  fast_two_sum (h, l, __builtin_fma (ee, log2_h, l1), z);
  /* here |hh+l1|+|z| <= 3275606777621385*2^-42 + 0.0022 < 745
     thus |h| < 745, and the additional error from the fast_two_sum() call is
     bounded by 2^-105*745 < 2^-95.4. */
  /* add ph + pl + l2 to l */
  *l = ph + (*l + (l2 + pl));
  /* here |ph| < 2.26e-6, |l| < ulp(h) = 2^-43, |l2| < 2^-43 and
     |pl| < 2^-69.99, thus |l2 + pl| < 2^-42 and |*l + l2 + pl| < 2^-41.99,
     and the rounding error on l2 + pl is bounded by 2^-95 (l2 + pl cannot
     be > 2^-42), and that on *l + (...) by 2^-94.
     Now |ph + (*l + (l2 + pl))| < 2.26e-6 + 2^-41.99 < 2^-18.7, thus the
     rounding error on ph + ... is bounded by ulp(2^-18.7) = 2^-71, which
     yields a cumulated error bound of 2^-71 + 2^-95 + 2^-94 < 2^-70.99. */

  *l = __builtin_fma (ee, log2_l, *l);
  /* let l_in be the input value of *l, and l_out the output value.
     We have |l_in| < 2^-18.7 (from above)
     and |e*log2_l| <= 1074*0x1.ef35793c7673p-45
     thus |l_out| < 2^-18.69 and err(l_out) <= ulp(2^-18.69) = 2^-71 */

  /* The absolute error on h + l is bounded by:
     2^-78.25 from the error in the Sollya polynomial plus the rounding errors
              in p_1 (&ph, &pl, z)
     2^-91.94 for the maximal difference |e*(log(2)-(log2_h + log2_l))|
              (|e| <= 1074 and |log(2)-(log2_h + log2_l)| < 2^-102.01)
     2^-97 for the maximal difference |l1 + l2 - (-log(r))|
     2^-95.4 from the fast_two_sum call
     2^-70.99 from the *l = ph + (*l + l2) instruction
     2^-71 from the last __builtin_fma call.
     This gives an absolute error bounded by < 2^-69.99.
  */
}

static void log_2 (dint64_t *r, dint64_t *x);

/* INVLOG2H+INVLOG2L is a double-double approximation of 1/log(2):
   | INVLOG2H + INVLOG2L - 1/log(2) | < 2^-109.53 */
#define INVLOG2H 0x1.71547652b82fep+0
#define INVLOG2L 0x1.777d0ffda0d24p-56

/* deal with |x| < 2^-900, then log2p1(x) ~ x/log(2) */
static double
cr_log2p1_accurate_tiny (double x)
{
  double h, l;

  // exceptional values
  if (__builtin_fabs (x) == 0x0.2c316a14459d8p-1022) {
#ifdef CORE_MATH_SUPPORT_ERRNO
    errno = ERANGE; // underflow
#endif
    return (x > 0) ?__builtin_fma (0x1p-600, 0x1p-600, 0x1.fe0e7458ac1f8p-1025)
      : __builtin_fma (-0x1p-600, 0x1p-600, -0x1.fe0e7458ac1f8p-1025);
  }

  /* first scale x to avoid truncation of l in the underflow region */
  double sx = x * 0x1p106;
  s_mul (&h, &l, sx, INVLOG2H, INVLOG2L);
  double res = (h + l) * 0x1p-106; // expected result
  l = __builtin_fma (-res, 0x1p106, h) + l;
  // the correction to apply to res is l*2^-106
  /* For all rounding modes, we have underflow
     for |x| <= 0x1.62e42fefa39eep-1023 */
#ifdef CORE_MATH_SUPPORT_ERRNO
  if (__builtin_fabs (x) <= 0x1.62e42fefa39eep-1023)
    errno = ERANGE; // underflow
#endif
  return __builtin_fma (l, 0x1p-106, res);
}

/* the following is a degree-17 polynomial approximating log2p1(x) for
   |x| <= 2^-5 with relative error < 2^-105.02, cf log2p1_accurate.sollya */
static const double Pacc[] = {
  0x1.71547652b82fep+0, 0x1.777d0ffda0d24p-56,   // degree 1: Pacc[0], Pacc[1]
  -0x1.71547652b82fep-1, -0x1.777d0ffd9ddb8p-57, // degree 2: Pacc[2], Pacc[3]
  0x1.ec709dc3a03fdp-2, 0x1.d27f055481523p-56,   // degree 3: Pacc[4], Pacc[5]
  -0x1.71547652b82fep-2, -0x1.777d1456a14c4p-58, // degree 4: Pacc[6], Pacc[7]
  0x1.2776c50ef9bfep-2, 0x1.e4b2a04f81513p-56,   // degree 5: Pacc[8], Pacc[9]
  -0x1.ec709dc3a03fdp-3, -0x1.d2072e751087ap-57, // degree 6: Pacc[10], Pacc[11]
  0x1.a61762a7aded9p-3, 0x1.f90f4895378acp-58,   // degree 7: Pacc[12], Pacc[13]
  -0x1.71547652b8301p-3,                         // degree 8: Pacc[14]
  0x1.484b13d7c02aep-3,                          // degree 9: Pacc[15]
  -0x1.2776c50ef7591p-3,                         // degree 10: Pacc[16]
  0x1.0c9a84993cabbp-3,                          // degree 11: Pacc[17]
  -0x1.ec709de7b1612p-4,                         // degree 12: Pacc[18]
  0x1.c68f56ba73fd1p-4,                          // degree 13: Pacc[19]
  -0x1.a616c83da87e7p-4,                         // degree 14: Pacc[20]
  0x1.89f3042097218p-4,                          // degree 15: Pacc[21]
  -0x1.72b376930a3fap-4,                         // degree 16: Pacc[22]
  0x1.5d0211d5ab53p-4,                           // degree 17: Pacc[23]
};

/* deal with 2^-900 <= x < 2^-5, using the polynomial Pacc */
static double
cr_log2p1_accurate_small (double x)
{
  double h, l, t;

#define EXCEPTIONS 247
  /* The following table contains entries (x,h) with x an exceptional case
     and h the rounding to nearest of log2p1(x). It should be sorted by
     increasing values of x. */
static const double exceptions[EXCEPTIONS][2] = {
    {-0x1.f5baee010ccc6p-6, -0x1.6f94484e5e1fdp-5},
    {-0x1.98e2aeed83e64p-7, -0x1.28cddfa8611e5p-6},
    {-0x1.7ff01253739d1p-7, -0x1.16967dd4eee8ep-6},
    {-0x1.ddabc67cec3c1p-9, -0x1.593238c1d2c35p-8},
    {-0x1.921c00a04d8a9p-9, -0x1.2281a6c541e39p-8},
    {-0x1.ae85103cce3e5p-10, -0x1.36cf465265029p-9},
    {-0x1.5955d05fd828cp-10, -0x1.f28ab65bc3763p-10},
    {-0x1.3623eff91de91p-10, -0x1.bfb3efcdf2bc4p-10},
    {-0x1.73ccf8ee62819p-12, -0x1.0c3ebbd1a501fp-11},
    {-0x1.fa5579e9a6941p-16, -0x1.6d3f8a9ba2e55p-15},
    {-0x1.75e4f1fb4d0f9p-17, -0x1.0db590d8385dep-16},
    {-0x1.b53dee1a96ca4p-18, -0x1.3b6786a5a9a69p-17},
    {-0x1.c66cb81b37ebbp-19, -0x1.47cc75e849dfcp-18},
    {-0x1.a1b96d88621b3p-19, -0x1.2d532b20e90b5p-18},
    {-0x1.bdb50ccf06b7ap-20, -0x1.418293ee25cd9p-19},
    {-0x1.77d66368613b4p-21, -0x1.0f1c08e43f217p-20},
    {-0x1.752326c780f68p-21, -0x1.0d296993d1368p-20},
    {-0x1.40e3f4e392fc8p-24, -0x1.cef2743b49f63p-24},
    {-0x1.16bba98a001c1p-24, -0x1.922076a30742p-24},
    {-0x1.51fb235627c5cp-28, -0x1.e79a80a1dd85bp-28},
    {-0x1.9b1805f30b4edp-31, -0x1.288aa13d73d39p-30},
    {-0x1.37a6f3079625dp-31, -0x1.c19e772379002p-31},
    {-0x1.672dad63d9469p-32, -0x1.0317abf72cb8p-31},
    {-0x1.025b55debc13p-32, -0x1.74bae4288af9fp-32},
    {-0x1.3278a26ed0162p-34, -0x1.ba24ff5dea796p-34},
    {-0x1.d88aedf0484a7p-36, -0x1.54de147b2643bp-35},
    {-0x1.6d4141037f32dp-37, -0x1.0779c878c2738p-36},
    {-0x1.40739786837bp-40, -0x1.ce50577370365p-40},
    {-0x1.ba43bd8edc2a3p-42, -0x1.3f06c76fa0047p-41},
    {-0x1.5892a86231e56p-42, -0x1.f11d1417baf8bp-42},
    {-0x1.21b2c54479c4dp-42, -0x1.a1f242e670d73p-42},
    {-0x1.526d1450f8572p-43, -0x1.e83ee27f027afp-43},
    {-0x1.bab64e105226ap-45, -0x1.3f596b7b1c5e2p-44},
    {-0x1.e9c2de7f3aa23p-47, -0x1.6149bc823c50fp-46},
    {-0x1.eaba84e0bd618p-49, -0x1.61fc60c60a648p-48},
    {-0x1.2516dafdf17adp-50, -0x1.a6d6a49f2187fp-50},
    {-0x1.9914a112c8dadp-51, -0x1.2716da024f6d9p-50},
    {-0x1.a052e3d5e791p-52, -0x1.2c506b0368099p-51},
    {-0x1.92ba5fbf0a946p-54, -0x1.2281c1a6e942bp-53},
    {-0x1.6f332a62b59f9p-56, -0x1.08e0f34d36269p-55},
    {-0x1.21530a306cc85p-56, -0x1.a16826a8e825dp-56},
    {-0x1.292f4e5d19917p-57, -0x1.acbf3cd5bd7eap-57},
    {-0x1.12b592f889516p-59, -0x1.8c525b64ed08ep-59},
    {-0x1.7780d5e5cf5c5p-60, -0x1.0ede4c1293a9bp-59},
    {-0x1.32ed98e196cf5p-60, -0x1.bacdbd3005cd7p-60},
    {-0x1.d73ad29747834p-61, -0x1.53eba1534455ap-60},
    {-0x1.354976dca453cp-61, -0x1.be34ef62d61b1p-61},
    {-0x1.1483eb37c9829p-61, -0x1.8eed6122bf62fp-61},
    {-0x1.fa53cebe640ccp-62, -0x1.6d3ced43408f1p-61},
    {-0x1.6923c31228cd3p-62, -0x1.0481d96a2dfcap-61},
    {-0x1.796ad95f38488p-63, -0x1.103fc46963aafp-62},
    {-0x1.470187381f305p-63, -0x1.d7c5178ca1765p-63},
    {-0x1.9234c78cc23acp-64, -0x1.2221636cd4353p-63},
    {-0x1.c7c8b78d5a272p-65, -0x1.48c7588c6213bp-64},
    {-0x1.a595e9f7d6e25p-65, -0x1.301c17252a9f4p-64},
    {-0x1.47aea7608c02bp-65, -0x1.d8bedc057858cp-65},
    {-0x1.1b19925f9fc06p-65, -0x1.986d43391ffdp-65},
    {-0x1.d62c98023cdbap-66, -0x1.5328b386ca8b2p-65},
    {-0x1.3b65311720a11p-66, -0x1.c704eabcff94fp-66},
    {-0x1.026e1ee1c55aep-68, -0x1.74d5fe17d2fe3p-68},
    {-0x1.cf96e02037643p-70, -0x1.4e68b66aa8cbbp-69},
    {-0x1.b9ab928f8614ap-70, -0x1.3e99035861d26p-69},
    {-0x1.297ee84082dafp-70, -0x1.ad3213e9d1878p-70},
    {-0x1.f63a8ce2364f3p-71, -0x1.6a480c34c7c5bp-70},
    {-0x1.ed7ff50423e4dp-71, -0x1.63fc211fe70aap-70},
    {-0x1.6856506d8234ap-71, -0x1.03eda6663a4b2p-70},
    {-0x1.62a8f9300de6fp-71, -0x1.ffaa928c83f63p-71},
    {-0x1.de8c6ef8a4dcep-73, -0x1.5933279c15d49p-72},
    {-0x1.698f4eb77100ap-73, -0x1.04cf6d4c9032bp-72},
    {-0x1.5536e12eb7335p-74, -0x1.ec44ae4bc644p-74},
    {-0x1.2b19ae19e4f3ep-74, -0x1.af82b29eef2ecp-74},
    {-0x1.7711d03d5c7bdp-75, -0x1.0e8e362e16fe6p-74},
    {-0x1.6104ba44449ep-75, -0x1.fd4c4933c4aadp-75},
    {-0x1.15108908835fp-75, -0x1.8fb83f0161f49p-75},
    {-0x1.1288bfd6f822ap-76, -0x1.8c11b0478bf38p-76},
    {-0x1.dbf55aa9a7eb9p-77, -0x1.5754d81696c42p-76},
    {-0x1.a175bf47b2093p-79, -0x1.2d223a2622b15p-78},
    {-0x1.75817dcf08a91p-81, -0x1.0d6d70bb89ce9p-80},
    {-0x1.6340571968b67p-82, -0x1.0042796d534fdp-81},
    {-0x1.06501e96466cfp-83, -0x1.7a70079e554fdp-83},
    {-0x1.86e6a6125ee5fp-85, -0x1.19f9b6ddcc3e9p-84},
    {-0x1.ce9392feefd4dp-86, -0x1.4dadaa8d9897dp-85},
    {-0x1.60fa7c8eee569p-86, -0x1.fd3d82e0680cdp-86},
    {-0x1.30876af870c12p-86, -0x1.b757aa6005d47p-86},
    {-0x1.1cf977003bdd6p-86, -0x1.9b219a1974289p-86},
    {-0x1.2c23f2bc02277p-87, -0x1.b102d7663223fp-87},
    {-0x1.05ae004891094p-87, -0x1.7986247373017p-87},
    {-0x1.b86b45d2c7c74p-88, -0x1.3db1f733be456p-87},
    {-0x1.63c4a608aac21p-89, -0x1.00a1ea24493b7p-88},
    {-0x1.ffc87ce076dfap-91, -0x1.712c6b2a267f5p-90},
    {-0x1.127630515eb89p-91, -0x1.8bf6e9484dd46p-91},
    {-0x1.db7daf1e016dfp-92, -0x1.56fe8536a47e9p-91},
    {-0x1.ca7e01c959788p-92, -0x1.4abb72eb058bep-91},
    {-0x1.4106468e7b671p-94, -0x1.cf23f6232620ep-94},
    {-0x1.cd79e208d957ep-95, -0x1.4ce2780c1bb7bp-94},
    {-0x1.0c5a1aeb10a7cp-95, -0x1.83266a65e67b3p-95},
    {-0x1.de8de3ca3cb0fp-97, -0x1.5934348aa44c5p-96},
    {-0x1.8ba5360a2b553p-99, -0x1.1d65d5fc31246p-98},
    {-0x1.618b50a22cecp-105, -0x1.fe0e7458ac1fcp-105},
    {-0x1.09287c79a1b1p-105, -0x1.7e8ad7428117dp-105},
    {-0x1.618b50a22cecp-106, -0x1.fe0e7458ac1fcp-106},
    {-0x1.09287c79a1b1p-106, -0x1.7e8ad7428117dp-106},
    {0x1.198cc57cb19cfp-106, 0x1.9630ccfb659e1p-106},
    {0x1.71ef99a53cd7fp-106, 0x1.0ada3508c853p-105},
    {0x1.198cc57cb19cfp-105, 0x1.9630ccfb659e1p-105},
    {0x1.71ef99a53cd7fp-105, 0x1.0ada3508c853p-104},
    {0x1.198cc57cb19cfp-104, 0x1.9630ccfb659e1p-104},
    {0x1.71ef99a53cd7fp-104, 0x1.0ada3508c853p-103},
    {0x1.198cc57cb19cfp-103, 0x1.9630ccfb659e1p-103},
    {0x1.8253e2a84cc3ep-103, 0x1.16ad2fe53a962p-102},
    {0x1.dab6b6d0d7feep-103, 0x1.566efe70501a1p-102},
    {0x1.29f10e7fc188ep-102, 0x1.add6c2b44a245p-102},
    {0x1.92b82bab5cafdp-102, 0x1.22802ac1acd94p-101},
    {0x1.4ab9a085e160cp-101, 0x1.dd22ae261330dp-101},
    {0x1.d4494fb79c5f9p-101, 0x1.51cc163375e5cp-100},
    {0x1.e4ad98baac4b8p-101, 0x1.5d9f110fe828ep-100},
    {0x1.f511e1bdbc377p-101, 0x1.69720bec5a6cp-100},
    {0x1.ebc08e5bdda62p-99, 0x1.62b965d563ddcp-98},
    {0x1.6c27c113f6f3fp-98, 0x1.06aeb9548f9f3p-97},
    {0x1.af040230221a5p-97, 0x1.36e97dd79baccp-96},
    {0x1.e743d2da8338cp-97, 0x1.5f7cc3326e2bp-96},
    {0x1.34e55b50b5476p-95, 0x1.bda482a6d3947p-95},
    {0x1.4265b80b22415p-95, 0x1.d11efcab7f66ep-95},
    {0x1.4fe614c58f3b4p-95, 0x1.e49976b02b395p-95},
    {0x1.7bf0a5c56ed66p-95, 0x1.12119cc1d532cp-94},
    {0x1.d896d7b343ddap-94, 0x1.54e6ac6b33b3ap-93},
    {0x1.4b7dadb0f0318p-93, 0x1.de3d85d01317bp-93},
    {0x1.6201aba46fb8p-93, 0x1.feb934937f571p-93},
    {0x1.05221bfa8a0fap-92, 0x1.78bc523768e9ap-92},
    {0x1.3d5b61f8af06p-91, 0x1.c9d96ce244a42p-91},
    {0x1.572552ca58e6ep-91, 0x1.ef0e032ee98acp-91},
    {0x1.3a41b77e1cfcap-90, 0x1.c560684cd2e49p-90},
    {0x1.94d5d40edd1e3p-90, 0x1.2406e3f8308dap-89},
    {0x1.02f8c70e9f28p-89, 0x1.759e083fbdf8ap-89},
    {0x1.61de0b2528516p-89, 0x1.fe85ce7ae82bp-89},
    {0x1.64cc21a428f53p-89, 0x1.015ffa3105406p-88},
    {0x1.31cdce0e38005p-88, 0x1.b92e8ae1c84e8p-88},
    {0x1.5b9ab5b571503p-88, 0x1.f57caf57c945dp-88},
    {0x1.7654fc15c252dp-88, 0x1.0e0600211a7d8p-87},
    {0x1.abf231afdb1f5p-88, 0x1.34b2a57159769p-87},
    {0x1.8586b6e5cbb06p-87, 0x1.18fbd8ef99db2p-86},
    {0x1.ae2b54125b144p-87, 0x1.364d309c81b79p-86},
    {0x1.1d640ce51c6b5p-86, 0x1.9bbb5f52ce771p-86},
    {0x1.3a0e97e84bb1ep-86, 0x1.c516a6e7e1fb3p-86},
    {0x1.742efa2dfc234p-86, 0x1.0c79410624d52p-85},
    {0x1.63eb00801d3d6p-85, 0x1.00bd94ab13e1ep-84},
    {0x1.a8b9097e3a9aep-85, 0x1.325f6bda4b8f1p-84},
    {0x1.807d37ffa6069p-83, 0x1.1559ac41e95fdp-82},
    {0x1.3528bc4882f1fp-82, 0x1.be05b7a6f8ce9p-82},
    {0x1.982a8e52379abp-82, 0x1.266e00e15b035p-81},
    {0x1.317c7aec05f2cp-81, 0x1.b8b93738a057ap-81},
    {0x1.4376f83fcc3c7p-81, 0x1.d2a9345ec5951p-81},
    {0x1.569aad73361dap-80, 0x1.ee45fd1dfacc6p-80},
    {0x1.575476e5e84cp-80, 0x1.ef5205c84626cp-80},
    {0x1.f20aa391bfeabp-80, 0x1.6742d3ba4b4b5p-79},
    {0x1.181b21bfde59bp-79, 0x1.941b8603a63b6p-79},
    {0x1.0934d6a88672dp-78, 0x1.7e9ca95365c5ap-78},
    {0x1.0f9c8867c5241p-78, 0x1.87da3d843fa48p-78},
    {0x1.4c5324339bb48p-78, 0x1.df717c0ff8c99p-78},
    {0x1.f7d8b04b61989p-78, 0x1.6b72c90b97cb1p-77},
    {0x1.8f27d6bf246acp-75, 0x1.1fee0f0ca892p-74},
    {0x1.f83edae3aa93fp-75, 0x1.6bbc7b98f1b2ep-74},
    {0x1.cda9fe2248ec6p-74, 0x1.4d052c3c2b0e3p-73},
    {0x1.c0ee5d2d2d30ap-73, 0x1.43d5d901cfa0fp-72},
    {0x1.628c0b5b4b9bbp-72, 0x1.ff80d6316f1efp-72},
    {0x1.f018c61839599p-72, 0x1.65dbb17ad1651p-71},
    {0x1.87a0cd7eeca98p-71, 0x1.1a7ffefd05629p-70},
    {0x1.5c5f1081b8abdp-70, 0x1.f697f700c874dp-70},
    {0x1.77e590243e48fp-69, 0x1.0f26f4ea680bep-68},
    {0x1.c943ea0da88dfp-69, 0x1.49d8e0ffc588cp-68},
    {0x1.d33e23beb1528p-68, 0x1.510b5cef9b5bcp-67},
    {0x1.186615833f8f7p-67, 0x1.9487a81ae609dp-67},
    {0x1.4891077ffbab6p-66, 0x1.da057342f40fbp-66},
    {0x1.4193614150725p-64, 0x1.cfef882ce5ca8p-64},
    {0x1.572569359da9p-64, 0x1.ef0e2386ed35dp-64},
    {0x1.6a5f8252d9f3bp-64, 0x1.05659cd6a65d1p-63},
    {0x1.7b5eedadbdbd6p-64, 0x1.11a87f8ae134dp-63},
    {0x1.b2669d6e81f95p-64, 0x1.395a9dafcdcabp-63},
    {0x1.02754e0d621p-63, 0x1.74e05b747c3f1p-63},
    {0x1.42c2504353e38p-63, 0x1.d1a492a13d773p-63},
    {0x1.9a8cb37e9acd5p-63, 0x1.28262152f2ee4p-62},
    {0x1.bd1c7919023cp-63, 0x1.411472cfddceap-62},
    {0x1.e505bd69056d9p-63, 0x1.5ddea609f2b25p-62},
    {0x1.2dbbb07d596fep-62, 0x1.b34f1670999b4p-62},
    {0x1.9a2e439736a07p-62, 0x1.27e2021880394p-61},
    {0x1.112c460eafdcp-61, 0x1.8a1af1c4d551fp-61},
    {0x1.7dcdac46803eep-61, 0x1.1369b6a328fcdp-60},
    {0x1.4b2ed4f196fe5p-60, 0x1.ddcbc560b3595p-60},
    {0x1.bc6526d12f2a9p-60, 0x1.409035c53cd34p-59},
    {0x1.95242afed5822p-59, 0x1.243f668c8121dp-58},
    {0x1.158d3212d17dfp-58, 0x1.906c17ca52236p-58},
    {0x1.f2bb88afdad28p-58, 0x1.67c26e08f6021p-57},
    {0x1.057d5711bfd9ep-57, 0x1.793ff07d6bc71p-57},
    {0x1.18cebea44da63p-57, 0x1.951ea66b69f7fp-57},
    {0x1.4cd45d85f97bp-57, 0x1.e02bea523b28cp-57},
    {0x1.596a35cd55af7p-57, 0x1.f2540e0d3138dp-57},
    {0x1.9b7e25fae77ffp-56, 0x1.28d44c2a1cfe7p-55},
    {0x1.67c00f810444cp-55, 0x1.038143d2697e2p-54},
    {0x1.e62ce3c0305e7p-54, 0x1.5eb38ddc44f75p-53},
    {0x1.b2078ba8f6835p-52, 0x1.391609b20beaap-51},
    {0x1.cbe169f09f95ep-52, 0x1.4bbbd21c8c721p-51},
    {0x1.9ce011cf5f46ep-51, 0x1.29d3990338b4ep-50},
    {0x1.9b37cdd819b7cp-48, 0x1.28a18e0a504b8p-47},
    {0x1.ed87135915039p-48, 0x1.640143a8bde38p-47},
    {0x1.389bb3cfc42c2p-47, 0x1.c2ff91c241999p-47},
    {0x1.56cd7d201444cp-46, 0x1.ee8f4b39c7db4p-46},
    {0x1.fc53c8d303c2p-46, 0x1.6eae3d74765fbp-45},
    {0x1.396ae5fe7edc6p-44, 0x1.c42a7d73d80c7p-44},
    {0x1.57c17dadff53fp-44, 0x1.efef50874d46dp-44},
    {0x1.8e05d9bf54ff5p-43, 0x1.1f1ce05fc5585p-42},
    {0x1.7a55eea45459dp-39, 0x1.10e95805820a9p-38},
    {0x1.d204a5710c6b4p-38, 0x1.502939b175bc4p-37},
    {0x1.0477b177d47a9p-37, 0x1.77c676774427ep-37},
    {0x1.912cc5483c4bbp-37, 0x1.2162f23d082d5p-36},
    {0x1.32ba6bfc6df61p-36, 0x1.ba83e89742ee7p-36},
    {0x1.b881f93d3d7bcp-35, 0x1.3dc2574084694p-34},
    {0x1.33c28a8adb35dp-33, 0x1.bc00f3c633b0ap-33},
    {0x1.1bab50d82fdf7p-30, 0x1.993f86d739365p-30},
    {0x1.5b8a987b105b9p-29, 0x1.f5656fdaf6efcp-29},
    {0x1.a7915a8ef7252p-29, 0x1.318a2177ecf63p-28},
    {0x1.51cba0164bed2p-27, 0x1.e755f4897c124p-27},
    {0x1.1859f2f12c686p-26, 0x1.9476260e06ba5p-26},
    {0x1.5004cdd3ec269p-26, 0x1.e4c5c9414182ep-26},
    {0x1.970634f93920ap-25, 0x1.259b1dcdcb00fp-24},
    {0x1.446805a9fe779p-23, 0x1.d404f5eb9474dp-23},
    {0x1.4f86281412f0bp-21, 0x1.e40f08f4a16c4p-21},
    {0x1.53e1ee4cd607ep-21, 0x1.ea58c18ad7665p-21},
    {0x1.91848ab4215a3p-20, 0x1.21a2344dd6315p-19},
    {0x1.7dbc650f5516cp-19, 0x1.135d264c757b5p-18},
    {0x1.5ed15c24566e4p-16, 0x1.fa1e29df34a0fp-16},
    {0x1.66e3866f48983p-14, 0x1.02df58d57f81fp-13},
    {0x1.4a82829ba33efp-13, 0x1.dcc98bad34bd4p-13},
    {0x1.ddacfe52ca881p-13, 0x1.5887eeb1071c7p-12},
    {0x1.cee52926a8cdep-11, 0x1.4dc2ce20fe5dbp-10},
    {0x1.50878a3127c53p-10, 0x1.e532b0254b97ep-10},
    {0x1.c60c6b10503bdp-10, 0x1.473e526fad0ap-9},
    {0x1.cb331784430e1p-10, 0x1.4af3e3d364e57p-9},
    {0x1.d19f229d1451dp-10, 0x1.4f93ba71b12bap-9},
    {0x1.15862abd340e8p-9, 0x1.8ff59817c7989p-9},
    {0x1.6d42e64d940acp-9, 0x1.071d292d678dfp-8},
    {0x1.df670b0e00641p-9, 0x1.592f57b46d345p-8},
    {0x1.fe604d486351fp-9, 0x1.6f719430581c3p-8},
    {0x1.0b896d69a6a9ap-7, 0x1.80682bd3a1419p-7},
    {0x1.c91984e0cebe7p-7, 0x1.4772f8c94aecdp-6},
    {0x1.cdfe4fcde7278p-7, 0x1.4aee2c1be2fb5p-6},
    {0x1.fa7ca1e176885p-7, 0x1.6a8ee6a3521fdp-6},
    {0x1.2cbfdc5da28d9p-6, 0x1.adf49cda3ad37p-6},
  };

/* the following table contains 0 if log2p1(x) is exact, -1 if it should be
   rounded down with respect to the value in the first table, and +1 if it
   should be rounded up */
static const int8_t exceptions_rnd[EXCEPTIONS] = {
    1, /* -0x1.f5baee010ccc6p-6 */
    1, /* -0x1.98e2aeed83e64p-7 */
    -1, /* -0x1.7ff01253739d1p-7 */
    1, /* -0x1.ddabc67cec3c1p-9 */
    -1, /* -0x1.921c00a04d8a9p-9 */
    -1, /* -0x1.ae85103cce3e5p-10 */
    1, /* -0x1.5955d05fd828cp-10 */
    -1, /* -0x1.3623eff91de91p-10 */
    -1, /* -0x1.73ccf8ee62819p-12 */
    1, /* -0x1.fa5579e9a6941p-16 */
    -1, /* -0x1.75e4f1fb4d0f9p-17 */
    1, /* -0x1.b53dee1a96ca4p-18 */
    1, /* -0x1.c66cb81b37ebbp-19 */
    1, /* -0x1.a1b96d88621b3p-19 */
    -1, /* -0x1.bdb50ccf06b7ap-20 */
    -1, /* -0x1.77d66368613b4p-21 */
    -1, /* -0x1.752326c780f68p-21 */
    -1, /* -0x1.40e3f4e392fc8p-24 */
    -1, /* -0x1.16bba98a001c1p-24 */
    1, /* -0x1.51fb235627c5cp-28 */
    -1, /* -0x1.9b1805f30b4edp-31 */
    1, /* -0x1.37a6f3079625dp-31 */
    1, /* -0x1.672dad63d9469p-32 */
    1, /* -0x1.025b55debc13p-32 */
    -1, /* -0x1.3278a26ed0162p-34 */
    1, /* -0x1.d88aedf0484a7p-36 */
    -1, /* -0x1.6d4141037f32dp-37 */
    -1, /* -0x1.40739786837bp-40 */
    1, /* -0x1.ba43bd8edc2a3p-42 */
    1, /* -0x1.5892a86231e56p-42 */
    -1, /* -0x1.21b2c54479c4dp-42 */
    1, /* -0x1.526d1450f8572p-43 */
    1, /* -0x1.bab64e105226ap-45 */
    1, /* -0x1.e9c2de7f3aa23p-47 */
    1, /* -0x1.eaba84e0bd618p-49 */
    1, /* -0x1.2516dafdf17adp-50 */
    -1, /* -0x1.9914a112c8dadp-51 */
    1, /* -0x1.a052e3d5e791p-52 */
    1, /* -0x1.92ba5fbf0a946p-54 */
    1, /* -0x1.6f332a62b59f9p-56 */
    1, /* -0x1.21530a306cc85p-56 */
    1, /* -0x1.292f4e5d19917p-57 */
    1, /* -0x1.12b592f889516p-59 */
    1, /* -0x1.7780d5e5cf5c5p-60 */
    1, /* -0x1.32ed98e196cf5p-60 */
    1, /* -0x1.d73ad29747834p-61 */
    1, /* -0x1.354976dca453cp-61 */
    1, /* -0x1.1483eb37c9829p-61 */
    1, /* -0x1.fa53cebe640ccp-62 */
    1, /* -0x1.6923c31228cd3p-62 */
    1, /* -0x1.796ad95f38488p-63 */
    1, /* -0x1.470187381f305p-63 */
    1, /* -0x1.9234c78cc23acp-64 */
    -1, /* -0x1.c7c8b78d5a272p-65 */
    1, /* -0x1.a595e9f7d6e25p-65 */
    1, /* -0x1.47aea7608c02bp-65 */
    1, /* -0x1.1b19925f9fc06p-65 */
    1, /* -0x1.d62c98023cdbap-66 */
    -1, /* -0x1.3b65311720a11p-66 */
    1, /* -0x1.026e1ee1c55aep-68 */
    1, /* -0x1.cf96e02037643p-70 */
    1, /* -0x1.b9ab928f8614ap-70 */
    1, /* -0x1.297ee84082dafp-70 */
    1, /* -0x1.f63a8ce2364f3p-71 */
    1, /* -0x1.ed7ff50423e4dp-71 */
    1, /* -0x1.6856506d8234ap-71 */
    1, /* -0x1.62a8f9300de6fp-71 */
    1, /* -0x1.de8c6ef8a4dcep-73 */
    1, /* -0x1.698f4eb77100ap-73 */
    1, /* -0x1.5536e12eb7335p-74 */
    1, /* -0x1.2b19ae19e4f3ep-74 */
    1, /* -0x1.7711d03d5c7bdp-75 */
    1, /* -0x1.6104ba44449ep-75 */
    1, /* -0x1.15108908835fp-75 */
    1, /* -0x1.1288bfd6f822ap-76 */
    1, /* -0x1.dbf55aa9a7eb9p-77 */
    -1, /* -0x1.a175bf47b2093p-79 */
    -1, /* -0x1.75817dcf08a91p-81 */
    1, /* -0x1.6340571968b67p-82 */
    -1, /* -0x1.06501e96466cfp-83 */
    1, /* -0x1.86e6a6125ee5fp-85 */
    1, /* -0x1.ce9392feefd4dp-86 */
    1, /* -0x1.60fa7c8eee569p-86 */
    1, /* -0x1.30876af870c12p-86 */
    1, /* -0x1.1cf977003bdd6p-86 */
    1, /* -0x1.2c23f2bc02277p-87 */
    -1, /* -0x1.05ae004891094p-87 */
    1, /* -0x1.b86b45d2c7c74p-88 */
    1, /* -0x1.63c4a608aac21p-89 */
    1, /* -0x1.ffc87ce076dfap-91 */
    1, /* -0x1.127630515eb89p-91 */
    1, /* -0x1.db7daf1e016dfp-92 */
    1, /* -0x1.ca7e01c959788p-92 */
    1, /* -0x1.4106468e7b671p-94 */
    -1, /* -0x1.cd79e208d957ep-95 */
    1, /* -0x1.0c5a1aeb10a7cp-95 */
    -1, /* -0x1.de8de3ca3cb0fp-97 */
    1, /* -0x1.8ba5360a2b553p-99 */
    1, /* -0x1.618b50a22cecp-105 */
    1, /* -0x1.09287c79a1b1p-105 */
    1, /* -0x1.618b50a22cecp-106 */
    1, /* -0x1.09287c79a1b1p-106 */
    1, /* 0x1.198cc57cb19cfp-106 */
    1, /* 0x1.71ef99a53cd7fp-106 */
    1, /* 0x1.198cc57cb19cfp-105 */
    1, /* 0x1.71ef99a53cd7fp-105 */
    1, /* 0x1.198cc57cb19cfp-104 */
    1, /* 0x1.71ef99a53cd7fp-104 */
    1, /* 0x1.198cc57cb19cfp-103 */
    1, /* 0x1.8253e2a84cc3ep-103 */
    1, /* 0x1.dab6b6d0d7feep-103 */
    1, /* 0x1.29f10e7fc188ep-102 */
    1, /* 0x1.92b82bab5cafdp-102 */
    1, /* 0x1.4ab9a085e160cp-101 */
    -1, /* 0x1.d4494fb79c5f9p-101 */
    1, /* 0x1.e4ad98baac4b8p-101 */
    1, /* 0x1.f511e1bdbc377p-101 */
    1, /* 0x1.ebc08e5bdda62p-99 */
    1, /* 0x1.6c27c113f6f3fp-98 */
    1, /* 0x1.af040230221a5p-97 */
    1, /* 0x1.e743d2da8338cp-97 */
    1, /* 0x1.34e55b50b5476p-95 */
    1, /* 0x1.4265b80b22415p-95 */
    1, /* 0x1.4fe614c58f3b4p-95 */
    1, /* 0x1.7bf0a5c56ed66p-95 */
    1, /* 0x1.d896d7b343ddap-94 */
    1, /* 0x1.4b7dadb0f0318p-93 */
    1, /* 0x1.6201aba46fb8p-93 */
    1, /* 0x1.05221bfa8a0fap-92 */
    1, /* 0x1.3d5b61f8af06p-91 */
    1, /* 0x1.572552ca58e6ep-91 */
    1, /* 0x1.3a41b77e1cfcap-90 */
    1, /* 0x1.94d5d40edd1e3p-90 */
    1, /* 0x1.02f8c70e9f28p-89 */
    1, /* 0x1.61de0b2528516p-89 */
    1, /* 0x1.64cc21a428f53p-89 */
    1, /* 0x1.31cdce0e38005p-88 */
    1, /* 0x1.5b9ab5b571503p-88 */
    1, /* 0x1.7654fc15c252dp-88 */
    1, /* 0x1.abf231afdb1f5p-88 */
    1, /* 0x1.8586b6e5cbb06p-87 */
    -1, /* 0x1.ae2b54125b144p-87 */
    -1, /* 0x1.1d640ce51c6b5p-86 */
    1, /* 0x1.3a0e97e84bb1ep-86 */
    1, /* 0x1.742efa2dfc234p-86 */
    1, /* 0x1.63eb00801d3d6p-85 */
    1, /* 0x1.a8b9097e3a9aep-85 */
    1, /* 0x1.807d37ffa6069p-83 */
    -1, /* 0x1.3528bc4882f1fp-82 */
    1, /* 0x1.982a8e52379abp-82 */
    1, /* 0x1.317c7aec05f2cp-81 */
    1, /* 0x1.4376f83fcc3c7p-81 */
    1, /* 0x1.569aad73361dap-80 */
    1, /* 0x1.575476e5e84cp-80 */
    1, /* 0x1.f20aa391bfeabp-80 */
    1, /* 0x1.181b21bfde59bp-79 */
    1, /* 0x1.0934d6a88672dp-78 */
    1, /* 0x1.0f9c8867c5241p-78 */
    1, /* 0x1.4c5324339bb48p-78 */
    1, /* 0x1.f7d8b04b61989p-78 */
    1, /* 0x1.8f27d6bf246acp-75 */
    1, /* 0x1.f83edae3aa93fp-75 */
    1, /* 0x1.cda9fe2248ec6p-74 */
    1, /* 0x1.c0ee5d2d2d30ap-73 */
    -1, /* 0x1.628c0b5b4b9bbp-72 */
    1, /* 0x1.f018c61839599p-72 */
    1, /* 0x1.87a0cd7eeca98p-71 */
    1, /* 0x1.5c5f1081b8abdp-70 */
    1, /* 0x1.77e590243e48fp-69 */
    1, /* 0x1.c943ea0da88dfp-69 */
    1, /* 0x1.d33e23beb1528p-68 */
    1, /* 0x1.186615833f8f7p-67 */
    1, /* 0x1.4891077ffbab6p-66 */
    1, /* 0x1.4193614150725p-64 */
    1, /* 0x1.572569359da9p-64 */
    1, /* 0x1.6a5f8252d9f3bp-64 */
    1, /* 0x1.7b5eedadbdbd6p-64 */
    1, /* 0x1.b2669d6e81f95p-64 */
    1, /* 0x1.02754e0d621p-63 */
    1, /* 0x1.42c2504353e38p-63 */
    1, /* 0x1.9a8cb37e9acd5p-63 */
    1, /* 0x1.bd1c7919023cp-63 */
    1, /* 0x1.e505bd69056d9p-63 */
    1, /* 0x1.2dbbb07d596fep-62 */
    1, /* 0x1.9a2e439736a07p-62 */
    1, /* 0x1.112c460eafdcp-61 */
    1, /* 0x1.7dcdac46803eep-61 */
    -1, /* 0x1.4b2ed4f196fe5p-60 */
    1, /* 0x1.bc6526d12f2a9p-60 */
    1, /* 0x1.95242afed5822p-59 */
    1, /* 0x1.158d3212d17dfp-58 */
    1, /* 0x1.f2bb88afdad28p-58 */
    1, /* 0x1.057d5711bfd9ep-57 */
    1, /* 0x1.18cebea44da63p-57 */
    1, /* 0x1.4cd45d85f97bp-57 */
    1, /* 0x1.596a35cd55af7p-57 */
    -1, /* 0x1.9b7e25fae77ffp-56 */
    1, /* 0x1.67c00f810444cp-55 */
    1, /* 0x1.e62ce3c0305e7p-54 */
    1, /* 0x1.b2078ba8f6835p-52 */
    1, /* 0x1.cbe169f09f95ep-52 */
    1, /* 0x1.9ce011cf5f46ep-51 */
    1, /* 0x1.9b37cdd819b7cp-48 */
    1, /* 0x1.ed87135915039p-48 */
    1, /* 0x1.389bb3cfc42c2p-47 */
    -1, /* 0x1.56cd7d201444cp-46 */
    -1, /* 0x1.fc53c8d303c2p-46 */
    -1, /* 0x1.396ae5fe7edc6p-44 */
    -1, /* 0x1.57c17dadff53fp-44 */
    1, /* 0x1.8e05d9bf54ff5p-43 */
    1, /* 0x1.7a55eea45459dp-39 */
    -1, /* 0x1.d204a5710c6b4p-38 */
    -1, /* 0x1.0477b177d47a9p-37 */
    1, /* 0x1.912cc5483c4bbp-37 */
    -1, /* 0x1.32ba6bfc6df61p-36 */
    -1, /* 0x1.b881f93d3d7bcp-35 */
    1, /* 0x1.33c28a8adb35dp-33 */
    -1, /* 0x1.1bab50d82fdf7p-30 */
    -1, /* 0x1.5b8a987b105b9p-29 */
    1, /* 0x1.a7915a8ef7252p-29 */
    1, /* 0x1.51cba0164bed2p-27 */
    -1, /* 0x1.1859f2f12c686p-26 */
    1, /* 0x1.5004cdd3ec269p-26 */
    1, /* 0x1.970634f93920ap-25 */
    1, /* 0x1.446805a9fe779p-23 */
    -1, /* 0x1.4f86281412f0bp-21 */
    -1, /* 0x1.53e1ee4cd607ep-21 */
    1, /* 0x1.91848ab4215a3p-20 */
    1, /* 0x1.7dbc650f5516cp-19 */
    -1, /* 0x1.5ed15c24566e4p-16 */
    1, /* 0x1.66e3866f48983p-14 */
    -1, /* 0x1.4a82829ba33efp-13 */
    -1, /* 0x1.ddacfe52ca881p-13 */
    -1, /* 0x1.cee52926a8cdep-11 */
    1, /* 0x1.50878a3127c53p-10 */
    -1, /* 0x1.c60c6b10503bdp-10 */
    -1, /* 0x1.cb331784430e1p-10 */
    -1, /* 0x1.d19f229d1451dp-10 */
    -1, /* 0x1.15862abd340e8p-9 */
    -1, /* 0x1.6d42e64d940acp-9 */
    1, /* 0x1.df670b0e00641p-9 */
    1, /* 0x1.fe604d486351fp-9 */
    1, /* 0x1.0b896d69a6a9ap-7 */
    1, /* 0x1.c91984e0cebe7p-7 */
    -1, /* 0x1.cdfe4fcde7278p-7 */
    -1, /* 0x1.fa7ca1e176885p-7 */
    1, /* 0x1.2cbfdc5da28d9p-6 */
  };
  int a, b, c;
  for (a = 0, b = EXCEPTIONS; a + 1 != b;)
  {
    /* Invariant: if x is an exceptional case, we have
       exceptions[a][0] <= x and (x < exceptions[b][0] or b = EXCEPTIONS) */
    c = (a + b) / 2;
    if (exceptions[c][0] <= x)
      a = c;
    else
      b = c;
  }
  if (x == exceptions[a][0])
  {
    h = exceptions[a][1];
    int8_t del = (h > 0) ? exceptions_rnd[a] : -exceptions_rnd[a];
    return h + h * 0x1p-54 * (double) del;
  }
#undef EXCEPTIONS

  /* for degree 11 or more, ulp(c[d]*x^d) < 2^-105.5*|log2p1(x)|
     where c[d] is the degree-d coefficient of Pacc, thus we can compute
     with a double only */

  h = __builtin_fma (Pacc[23], x, Pacc[22]); // degree 16
  for (int i = 15; i >= 11; i--)
    h = __builtin_fma (h, x, Pacc[i+6]);        // degree i
  l = 0;
  for (int i = 10; i >= 8; i--)
    {
      s_mul (&h, &l, x, h, l);
      fast_two_sum (&h, &t, Pacc[i+6], h);
      l += t;
    }
  for (int i = 7; i >= 1; i--)
    {
      s_mul (&h, &l, x, h, l);
      fast_two_sum (&h, &t, Pacc[2*i-2], h);
      l += t + Pacc[2*i-1];
}
  /* final multiplication by x */
  s_mul (&h, &l, x, h, l);
  return h + l;
}

static inline double dint_tod (dint64_t *a);

/* accurate path, using Tom Hubrecht's code below */
static double
cr_log2p1_accurate (double x)
{
  dint64_t X, Y, C;
  double ax = __builtin_fabs (x);

  if (ax < 0x1p-5)
      return (ax < 0x1p-105) ? cr_log2p1_accurate_tiny (x)
        : cr_log2p1_accurate_small (x);

  /* (xh,xl) <- 1+x */
  double xh, xl;
  if (x > 1.0)
    fast_two_sum (&xh, &xl, x, 1.0);
  else
    fast_two_sum (&xh, &xl, 1.0, x);

  d64u64 t;
  /* log2p1(x) is exact when 1+x = 2^e, thus when 2^e-1 is exactly
     representable. This can only occur when xl=0 here. */
  if (xl == 0)
  {
    /* check if xh is a power of two */
    t.f = xh;
    if ((t.u << 12) == 0)
    {
      int e = (t.u >> 52) - 0x3ff;
      return (double) e;
    }
  }

  /* if x=2^e, the accurate path will fail for directed roundings */
  t.f = x;
  if ((t.u << 12) == 0)
    {
      int e = (t.u >> 52) - 0x3ff; // x = 2^e
      /* for e >= 49, log2p1(x) rounds to e for rounding to nearest;
         for e >= 48, log2p1(x) rounds to e for rounding toward zero;
         for e >= 48, log2p1(x) rounds to nextabove(e) for rounding up;
         for e >= 48, log2p1(x) rounds to e for rounding down. */
      if (e >= 49)
        return (double) e + 0x1p-48; // 0x1p-48 = 1/2 ulp(49)
    }

  dint_fromd (&X, xh, 0x3ff);
  log_2 (&Y, &X);

  div_dint_d (&C, xl, xh);
  mul_dint_126 (&X, &C, &C);
  /* multiply X by -1/2 */
  X.ex -= 1;
  X.sgn = 0x1;
  /* C <- C - C^2/2 */
  add_dint (&C, &C, &X);
  /* |C-log(1+xl/xh)| ~ 2e-64 */
  add_dint (&Y, &Y, &C);

  /* multiply by 1/log(2) */
  mul_dint_126 (&Y, &Y, &LOG2_INV);
  Y.ex -= 12; // since LOG2_INV approximates 2^12/log(2)

  return dint_tod_not_subnormal (&Y);
}

/* Given x > -1, put in (h,l) a double-double approximation of log2(1+x),
   and return a bound err on the maximal absolute error so that:
   |h + l - log2(1+x)| < err.
   We have x = m*2^e with 1 <= m < 2 (m = v.f) and -1074 <= e <= 1023.
   This routine is adapted from cr_log1p_fast.
*/
static double
cr_log2p1_fast (double *h, double *l, double x, int e, d64u64 v)
{
  if (e < -5) /* e <= -6 thus |x| < 2^-5 */
  {
    double lo;
    if (e <= -969)
    {
      /* then |x| might be as small as 2^-969, thus h=x/log(2) might in the
         binade [2^-969,2^-968), with ulp(h) = 2^-1021, and if |l| < ulp(h),
         then l.ulp() might be smaller than 2^-1074. We defer that case to
         the accurate path. */
      *h = *l = 0;
      return 1;
    }
    p_1a (h, &lo, x);
    fast_two_sum (h, l, x, *h);
    *l += lo;
    /* from analyze_x_plus_p1a(rel=true,Xmax=2^-5.) in the accompanying file
       log1p.sage, the relative error is bounded by 2^-61.14 with respect to
       h. We use the fact that we don't need the return value err to be
       positive, since we add/subtract it in the rounding test.
       We also get that the ratio |l/h| is bounded by 2^-50.96. */
    /* now we multiply h+l by 1/log(2) */
    d_mul (h, l, *h, *l, INVLOG2H, INVLOG2L);
    /* the d_mul() call decomposes into:
       a_mul (h_out, l1, h, INVLOG2H)
       l2 = __builtin_fma (h, INVLOG2L, l1)
       l_out = __builtin_fma (l, INVLOG2H, l2)
       we have |l1| <= ulp(h_out)
       since |INVLOG2L/INVLOG2H| < 2^-55, then |h*INVLOG2L| <= 2^-55*|h_out|
       and since |x| < 2^53*ulp(x): |h*INVLOG2L| <= ulp(h_out)/4
       thus |l2| <= 5/4*ulp(h_out).
       Now since |l/h| < 2^-50.96, |l*INVLOG2H| < 2^-50.96*|h*INVLOG2H|
       < 2^-50.96*(1+2^-52)*|h_out| < 2^-50.95*|h_out| < 4.15*ulp(h_out),
       thus |l_out| < o(4.15*ulp(h_out)+5/4*ulp(h_out)) < 5.5*ulp(h_out).
       The rounding errors are bounded by ulp(l2)+ulp(l_out)
       <= ulp(5/4*ulp(h_out)) + ulp(5.5*ulp(h_out))
       <= 2^-52*(5/4*ulp(h_out)+5.5*ulp(h_out)) [using ulp(x) <= 2^-52*|x|]
       <= 2^-49.2*ulp(h_out)
       We also have to take into account the ignored term l*INVLOG2L:
       |l*INVLOG2L| < 2^-50.96*|h|*2^-55.97*|INVLOG2H|
                    < 2^-106.93*(1+2^-52)*|h_out|
                    < 2^-106.92*|h_out|
                    < 2^-51.92*ulp(h_out) [using |x| < 2^53*ulp(x)]
      and the approximation error in INVLOG2H+INVLOG2L:
      |INVLOG2H + INVLOG2L - 1/log(2)| < 2^-110/log(2)
      The total error of d_mul() is thus bounded by:
      (2^-49.2+2^-51.92)*ulp(h_out) < 2^-48.99*ulp(h_out) < 2^-100.99*|h_out|,
      using again ulp(x) <= 2^-52*|x|.

      The relative error is thus bounded by
      (1+2^-61.14)*(1+2^-100.99)*(1+2^-110)-1 < 2^-61.13 */
    return 0x1.d4p-62 * *h; /* 2^-61.13 < 0x1.d4p-62 */
  }

  /* (xh,xl) <- 1+x */
  double xh, xl;
  if (x > 1.0) {
    if (__builtin_expect (x < 0x1.fffffffffffffp+1023, 1))
        fast_two_sum (&xh, &xl, x, 1.0);
    else // avoid spurious overflow for RNDU
      xh = x, xl = 1.0;
  }
  else
    fast_two_sum (&xh, &xl, 1.0, x);

  v.f = xh;
  e = (v.u >> 52) - 0x3ff;
  v.u = (0x3ffull << 52) | (v.u & 0xfffffffffffff);
  cr_log_fast (h, l, e, v);

  /* log(xh+xl) = log(xh) + log(1+xl/xh) */
  double c;
  if (__builtin_expect (xh <= 0x1p1022 || __builtin_fabs (xl) >= 4.0, 1))
    c = xl / xh;
  else
    c = 0; // avoid spurious underflow
  /* Since |xl| < ulp(xh), we have |xl| < 2^-52 |xh|,
     thus |c| < 2^-52, and since |log(1+x)-x| < x^2 for |x| < 0.5,
     we have |log(1+c)-c)| < c^2 < 2^-104. */
  *l += c;
  /* Since |l_in| < 2^-18.69 (from the analysis of cr_log_fast, see file
     ../log/log.c), and |c| < 2^-52, we have |l| < 2^-18.68, thus the
     rounding error in *l += c is bounded by ulp(2^-18.68) = 2^-71.
     The total absolute error is thus bounded by:
     0x1.b6p-69 + 2^-104 + 2^-71 < 2^-68.02. */

  /* now multiply h+l by 1/log(2) */
  d_mul (h, l, *h, *l, INVLOG2H, INVLOG2L);
  /* the d_mul() call decomposes into:
     a_mul (h_out, l1, h, INVLOG2H)
     l2 = __builtin_fma (h, INVLOG2L, l1)
     l_out = __builtin_fma (l, INVLOG2H, l2)
     We have three errors:
     * the rounding error in l2 = __builtin_fma (h, INVLOG2L, l1)
     * the rounding error in l_out = __builtin_fma (l, INVLOG2H, l2)
     * the ignored term l * INVLOG2L
     We have |h| < 745 thus |h*INVLOG2H| < 1075 thus |h_out| <= 1075
     and |l1| <= ulp(h_out) <= 2^-42.
     Then |h*INVLOG2L+l1| <= 745*INVLOG2L+2^-42 < 2^-41.9
     thus |l2| < 2^-41.9*(1+2^-52) < 2^-41.8
     and the first rounding error is bounded by ulp(2^-41.8) = 2^-94.
     Now |l*INVLOG2H+l2| < 2^-18.68*INVLOG2H+2^-41.8 < 2^-18.1
     thus |l_out| < 2^-18.1*(1+2^-52) < 2^-18.09
     and the second rounding error is bounded by ulp(2^-18.09) = 2^-71.
     The ignored term is bounded by |l*INVLOG2L| < 2^-18.68*INVLOG2L < 2^-74.1.
     Thus the absolute error from d_mul() is bounded by:
     2^-94 + 2^-71 + 2^-74.1 < 2^-70.84.

     Adding to the maximal absolute error of 2^-68.02 before d_mul(),
     we get 2^-68.02 + 2^-70.84 < 2^-67.82.
  */

  return 0x1.23p-68; /* 2^-67.82 < 0x1.23p-68 */
}


double
__log2p1 (double x)
{
  d64u64 v = {.f = x};
  int e = ((v.u >> 52) & 0x7ff) - 0x3ff;
  if (__builtin_expect (e == 0x400 || x == 0 || x <= -1.0, 0))
    /* case NaN/Inf, +/-0 or x <= -1 */
  {
    static const d64u64 minf = {.u = 0xfffull << 52};
    if (e == 0x400 && x != minf.f) /* NaN or + Inf*/
      return x + x;
    if (x <= -1.0) /* we use the fact that NaN < -1 is false */
    {
      /* log2p(x<-1) is NaN, log2p(-1) is -Inf and raises DivByZero */
      if (x < -1.0) {
#ifdef CORE_MATH_SUPPORT_ERRNO
	errno = EDOM;
#endif
        return 0.0 / 0.0;
      }
      else { // x=-1
#ifdef CORE_MATH_SUPPORT_ERRNO
	errno = ERANGE;
#endif
        return 1.0 / -0.0;
      }
    }
    return x + x; /* +/-0 */
  }

  /* now x > -1 */

  /* check x=2^n-1 for 0 <= n <= 53, where log2p1(x) is exact,
     and we shouldn't raise the inexact flag */
  if (0 <= e && e <= 52) {
    /* T[e]=2^(e+1)-1, i.e., the unique value of the form 2^n-1
       in the interval [2^e, 2^(e+1)). */
    static const double T[] = {
      0x1p+0, 0x1.8p+1, 0x1.cp+2, 0x1.ep+3, 0x1.fp+4, 0x1.f8p+5, 0x1.fcp+6, 0x1.fep+7, 0x1.ffp+8, 0x1.ff8p+9, 0x1.ffcp+10, 0x1.ffep+11, 0x1.fffp+12, 0x1.fff8p+13, 0x1.fffcp+14, 0x1.fffep+15, 0x1.ffffp+16, 0x1.ffff8p+17, 0x1.ffffcp+18, 0x1.ffffep+19, 0x1.fffffp+20, 0x1.fffff8p+21, 0x1.fffffcp+22, 0x1.fffffep+23, 0x1.ffffffp+24, 0x1.ffffff8p+25, 0x1.ffffffcp+26, 0x1.ffffffep+27, 0x1.fffffffp+28, 0x1.fffffff8p+29, 0x1.fffffffcp+30, 0x1.fffffffep+31, 0x1.ffffffffp+32, 0x1.ffffffff8p+33, 0x1.ffffffffcp+34, 0x1.ffffffffep+35, 0x1.fffffffffp+36, 0x1.fffffffff8p+37, 0x1.fffffffffcp+38, 0x1.fffffffffep+39, 0x1.ffffffffffp+40, 0x1.ffffffffff8p+41, 0x1.ffffffffffcp+42, 0x1.ffffffffffep+43, 0x1.fffffffffffp+44, 0x1.fffffffffff8p+45, 0x1.fffffffffffcp+46, 0x1.fffffffffffep+47, 0x1.ffffffffffffp+48, 0x1.ffffffffffff8p+49, 0x1.ffffffffffffcp+50, 0x1.ffffffffffffep+51, 0x1.fffffffffffffp+52};
    if (x == T[e])
      return e + 1;

  }

  /* For x=2^k-1, -53 <= k <= -1, log2p1(x) = k is also exact. */
  if (__builtin_expect (e == -1 && x < 0, 0)) { // -1 < x <= -1/2
    d64u64 w = {.f = 1.0 + x}; // 1+x is exact
    if ((w.u << 12) == 0) { // 1+x = 2^k
      int k = (w.u >> 52) - 0x3ff;
      return k;
    }
  }

  /* normalize v in [1,2) */
  v.u = (0x3ffull << 52) | (v.u & 0xfffffffffffff);
  /* now x = m*2^e with 1 <= m < 2 (m = v.f) and -1074 <= e <= 1023 */
  double h, l, err;
  err = cr_log2p1_fast (&h, &l, x, e, v);

  double left = h + (l - err), right = h + (l + err);
  if (left == right)
    return left;

  return cr_log2p1_accurate (x);
}
libm_alias_double (__log2p1, log2p1)

/* the following code was copied from Tom Hubrecht's implementation of
   correctly rounded pow for CORE-MATH */

// Approximation for the second iteration
static inline void p_2(dint64_t *r, dint64_t *z) {
  cp_dint(r, &P_2[0]);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[1], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[2], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[3], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[4], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[5], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[6], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[7], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[8], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[9], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[10], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[11], r);

  mul_dint_126(r, z, r);
  add_dint(r, &P_2[12], r);

  mul_dint_126(r, z, r);
}

static void log_2(dint64_t *r, dint64_t *x) {
  int64_t E = x->ex;

  // Find the lookup index
  uint16_t i = x->hi >> 55;

  if (x->hi > 0xb504f333f9de6484) {
    E++;
    i = i >> 1;
  }

  x->ex = x->ex - E;

  dint64_t z;
  mul_dint_126(&z, x, &_INVERSE_2[i - 128]);

  add_dint(&z, &M_ONE, &z);

  // E·log(2)
  mul_dint_2(r, E, &LOG2);

  dint64_t p;

  p_2(&p, &z);

  add_dint(&p, &_LOG_INV_2[i - 128], &p);

  add_dint(r, &p, r);
}
