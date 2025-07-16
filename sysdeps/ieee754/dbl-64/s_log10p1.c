/* Correctly rounded log10(1+x) for binary64 values.

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
  // clang sanitizer reports a failure with (_INVERSE-OFFSET)[i]
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

//static inline void dint_fromd (dint64_t *a, double b);
static void log_2 (dint64_t *r, dint64_t *x);
static inline double dint_tod (dint64_t *a);

/* INVLOG10H + INVLOG10L is a double-double approximation of 1/log(10):
   | INVLOG10H + INVLOG10L - 1/log(10) | < 2^-111.051 */
#define INVLOG10H 0x1.bcb7b1526e50ep-2
#define INVLOG10L 0x1.95355baaafad3p-57

/* deal with |x| < 2^-900, then log10p1(x) ~ x/log(10) */
static double
cr_log10p1_accurate_tiny (double x)
{
  double h, l;
  /* first scale x to avoid truncation of l in the underflow region */
  double sx = x * 0x1p106;
  s_mul (&h, &l, sx, INVLOG10H, INVLOG10L);
  double res = (h + l) * 0x1p-106; // expected result
  l = __builtin_fma (-res, 0x1p106, h) + l;
  // the correction to apply to res is l*2^-106
  /* For RNDN, we have underflow for |x| <= 0x1.26bb1bbb55515p-1021,
     and for rounding away, for |x| < 0x1.26bb1bbb55515p-1021. */
#ifdef CORE_MATH_SUPPORT_ERRNO
#define X0 0x1.26bb1bbb55515p-1021
  double dummy = __builtin_copysign (1.0, x);
  if (__builtin_fabs (x) < X0 ||
      (__builtin_fabs (x) == X0 && __builtin_fma (dummy, 0x1p-53, dummy) == dummy))
    errno = ERANGE; // underflow
#endif
  return __builtin_fma (l, 0x1p-106, res);
}

/* the following is a degree-17 polynomial approximating log10p1(x) for
   |x| <= 2^-5 with relative error < 2^-105.067, cf log10p1_accurate.sollya */
static const double Pacc[] = {
  0x1.bcb7b1526e50ep-2, 0x1.95355baaafad4p-57,   // degree 1: Pacc[0], Pacc[1]
  -0x1.bcb7b1526e50ep-3, -0x1.95355baaae078p-58, // degree 2: Pacc[2], Pacc[3]
  0x1.287a7636f435fp-3, -0x1.9c871838f83acp-58,  // degree 3: Pacc[4], Pacc[5]
  -0x1.bcb7b1526e50ep-4, -0x1.95355e23285f2p-59, // degree 4: Pacc[6], Pacc[7]
  0x1.63c62775250d8p-4, 0x1.442abd5831422p-59,   // degree 5: Pacc[8], Pacc[9]
  -0x1.287a7636f435fp-4, 0x1.9d116f225c4e4p-59,  // degree 6: Pacc[10], Pacc[11]
  0x1.fc3fa615105c7p-5, 0x1.4e1d7b479051p-61,    // degree 7: Pacc[12], Pacc[13]
  -0x1.bcb7b1526e512p-5, 0x1.9f884199ab0cep-59,  // degree 8: Pacc[14], Pacc[15]
  0x1.8b4df2f3f0486p-5,                          // degree 9: Pacc[16]
  -0x1.63c6277522391p-5,                         // degree 10: Pacc[17]
  0x1.436e526a79e5cp-5,                          // degree 11: Pacc[18]
  -0x1.287a764c5a762p-5,                         // degree 12: Pacc[19]
  0x1.11ac1e784daecp-5,                          // degree 13: Pacc[20]
  -0x1.fc3eedc920817p-6,                         // degree 14: Pacc[21]
  0x1.da5cac3522edbp-6,                          // degree 15: Pacc[22]
  -0x1.be5ca1f9a97cdp-6,                         // degree 16: Pacc[23]
  0x1.a44b64ca06e9bp-6,                          // degree 17: Pacc[24]
};

/* deal with 2^-900 <= x < 2^-5, using the polynomial Pacc */
static double
cr_log10p1_accurate_small (double x)
{
  double h, l, t;

  /* The following exceptional cases have at least 50 identical bits after
     the round bit, thus are hard to correctly round with double-double
     arithmetic. They should be sorted by increasing values of the first
     entry (x). */
#define EXCEPTIONS 222
static const double exceptions[EXCEPTIONS][2] = {
    {-0x1.e0648eff3dad8p-6, -0x1.a780e0727d64ap-7},
    {-0x1.8dd157e27ade5p-6, -0x1.5dcde532470b8p-7},
    {-0x1.739b846071578p-6, -0x1.467dac57cb711p-7},
    {-0x1.a3d1b9ce5737fp-7, -0x1.6f01a579312aap-8},
    {-0x1.6755766dea6c8p-8, -0x1.38f8e2796fe0bp-9},
    {-0x1.602d6acef178ap-8, -0x1.32b8e7a907ed3p-9},
    {-0x1.26ffe1529fb7p-8, -0x1.00cfd02e2d0cdp-9},
    {-0x1.be50b576d2dcbp-9, -0x1.8453776046421p-10},
    {-0x1.1279455abcf43p-9, -0x1.dd4f42c5de2f2p-11},
    {-0x1.0828a8bbb67a6p-9, -0x1.cb5a9a7b93968p-11},
    {-0x1.e5bc53490e628p-11, -0x1.a619a58f61455p-12},
    {-0x1.817b775a8e65cp-11, -0x1.4ef2e56d8cdcbp-12},
    {-0x1.cf968a9b81e41p-12, -0x1.92c1aa467dcb6p-13},
    {-0x1.13f6403cf8b55p-12, -0x1.df75428784266p-14},
    {-0x1.f5ec2799a555p-18, -0x1.b3f748ed03d0cp-19},
    {-0x1.b8e37862e8ea5p-20, -0x1.7ef3850122d41p-21},
    {-0x1.afbdf68019c4p-21, -0x1.7701ab9127b5ep-22},
    {-0x1.a7ab033c58b46p-23, -0x1.6ffe4f89d452fp-24},
    {-0x1.8c14a6409f43bp-23, -0x1.580804cea67d3p-24},
    {-0x1.9c74c219ae88fp-30, -0x1.66413aeaec9c9p-31},
    {-0x1.cb033247d6af5p-31, -0x1.8eb1743a11018p-32},
    {-0x1.8da6667882dcep-32, -0x1.5964f78f399p-33},
    {-0x1.8358a7e7e36dfp-34, -0x1.5071d9f1dcf1p-35},
    {-0x1.07a54aa3098b2p-36, -0x1.c9ffdb395aff8p-38},
    {-0x1.8b3f17bc52fep-37, -0x1.574e848546088p-38},
    {-0x1.066ca101bf96fp-38, -0x1.c7e0b4abbf63cp-40},
    {-0x1.be1f254f3131fp-39, -0x1.837f10fe1c9cbp-40},
    {-0x1.109e8e5411922p-39, -0x1.d9969d06f7365p-41},
    {-0x1.b169f3f07565bp-40, -0x1.787561041fadcp-41},
    {-0x1.ebee0a568ac68p-41, -0x1.ab48eaeabcc47p-42},
    {-0x1.4d089375ad275p-41, -0x1.2144ebc7d630dp-42},
    {-0x1.7ba91f831d839p-45, -0x1.49c4dfc38e44fp-46},
    {-0x1.ed9433cd12dfep-47, -0x1.acb79a558d2b2p-48},
    {-0x1.011108c573ad1p-49, -0x1.be92007347123p-51},
    {-0x1.b806fbada31eep-50, -0x1.7e33ed24c9f3cp-51},
    {-0x1.9dd7565d325e4p-52, -0x1.677536afc2p-53},
    {-0x1.84f828be9e9f3p-55, -0x1.51dac0be4752fp-56},
    {-0x1.fa19033240b03p-57, -0x1.b79743fd5936ap-58},
    {-0x1.5b97ae0171027p-57, -0x1.2dea3dfdd75aep-58},
    {-0x1.50060db4d7c31p-57, -0x1.23ddce6cea59ep-58},
    {-0x1.191ff3e3d56e7p-62, -0x1.e85d23901b0a5p-64},
    {-0x1.01f6630b953f2p-62, -0x1.c0206d974eb15p-64},
    {-0x1.1d16bd3e005a9p-63, -0x1.ef4000f154e9bp-65},
    {-0x1.f4e57e1eadaf2p-64, -0x1.b312b8df295c3p-65},
    {-0x1.eda036363f823p-64, -0x1.acc208bbc7387p-65},
    {-0x1.3ef3ee3f1171dp-66, -0x1.1509f74106523p-67},
    {-0x1.b356682c47d4cp-67, -0x1.7a211e7e17c48p-68},
    {-0x1.2684e330dcfcfp-67, -0x1.ffa1cf07ddc46p-69},
    {-0x1.17133b2d0251p-67, -0x1.e4cd9aaf8586bp-69},
    {-0x1.1223eea2a0873p-68, -0x1.dc3b0774c24e3p-70},
    {-0x1.ede211e678806p-69, -0x1.acfb3cdf6d2e1p-70},
    {-0x1.d148e85c20981p-70, -0x1.942428338e72cp-71},
    {-0x1.7829cd77bf34ep-71, -0x1.46bb3565cb62fp-72},
    {-0x1.041ec1d294952p-75, -0x1.c3dffe52cd2efp-77},
    {-0x1.bd58bcace9d65p-79, -0x1.82d2bb1242f9dp-80},
    {-0x1.b731f404a5e5fp-79, -0x1.7d7ae4123a038p-80},
    {-0x1.a55ec4a3bc202p-79, -0x1.6dff5bd45f9bap-80},
    {-0x1.97f1eac52ac07p-79, -0x1.625625d733d8dp-80},
    {-0x1.9110b1168be32p-79, -0x1.5c5c5defb02c9p-80},
    {-0x1.f1d2145ff142ep-84, -0x1.b066c8aac175fp-85},
    {-0x1.1f5de53a357d4p-84, -0x1.f3350cbc44c75p-86},
    {-0x1.06dca67f311a5p-86, -0x1.c8a34e92d126ep-88},
    {-0x1.e6f45d7354495p-87, -0x1.a6f69e074a2cfp-88},
    {-0x1.13c9a89fe407fp-87, -0x1.df17a4a414d02p-89},
    {-0x1.88fb6cbee3cf8p-89, -0x1.5557064517d0ep-90},
    {-0x1.0cd7d95208b86p-96, -0x1.d307456e6f031p-98},
    {-0x1.2cae25a71122fp-98, -0x1.052ae507f2c42p-99},
    {-0x1.1d4b74fda34aap-100, -0x1.ef9b957591849p-102},
    {0x1.2968419e1831fp-105, 0x1.02531caaf973p-106},
    {0x1.36568740cb558p-105, 0x1.0d8e60b26878p-106},
    {0x1.4344cce37e791p-105, 0x1.18c9a4b9d77dp-106},
    {0x1.50331286319cap-105, 0x1.2404e8c14682p-106},
    {0x1.5d215828e4c03p-105, 0x1.2f402cc8b587p-106},
    {0x1.6a0f9dcb97e3cp-105, 0x1.3a7b70d0248cp-106},
    {0x1.76fde36e4b075p-105, 0x1.45b6b4d79391p-106},
    {0x1.83ec2910fe2aep-105, 0x1.50f1f8df0296p-106},
    {0x1.90da6eb3b14e7p-105, 0x1.5c2d3ce6719bp-106},
    {0x1.9dc8b4566472p-105, 0x1.676880ede0ap-106},
    {0x1.aab6f9f917959p-105, 0x1.72a3c4f54fa5p-106},
    {0x1.b7a53f9bcab92p-105, 0x1.7ddf08fcbeaap-106},
    {0x1.c493853e7ddcbp-105, 0x1.891a4d042dafp-106},
    {0x1.d181cae131004p-105, 0x1.9455910b9cb4p-106},
    {0x1.de701083e423dp-105, 0x1.9f90d5130bb9p-106},
    {0x1.eb5e562697476p-105, 0x1.aacc191a7abep-106},
    {0x1.f84c9bc94a6afp-105, 0x1.b6075d21e9c3p-106},
    {0x1.029d70b5fec74p-104, 0x1.c142a12958c8p-106},
    {0x1.0f8bb658b1eadp-104, 0x1.d7b9293836d2p-106},
    {0x1.1c79fbfb650e6p-104, 0x1.ee2fb14714dcp-106},
    {0x1.2968419e1831fp-104, 0x1.02531caaf973p-105},
    {0x1.36568740cb558p-104, 0x1.0d8e60b26878p-105},
    {0x1.4344cce37e791p-104, 0x1.18c9a4b9d77dp-105},
    {0x1.50331286319cap-104, 0x1.2404e8c14682p-105},
    {0x1.5d215828e4c03p-104, 0x1.2f402cc8b587p-105},
    {0x1.6a0f9dcb97e3cp-104, 0x1.3a7b70d0248cp-105},
    {0x1.76fde36e4b075p-104, 0x1.45b6b4d79391p-105},
    {0x1.83ec2910fe2aep-104, 0x1.50f1f8df0296p-105},
    {0x1.90da6eb3b14e7p-104, 0x1.5c2d3ce6719bp-105},
    {0x1.9dc8b4566472p-104, 0x1.676880ede0ap-105},
    {0x1.aab6f9f917959p-104, 0x1.72a3c4f54fa5p-105},
    {0x1.b7a53f9bcab92p-104, 0x1.7ddf08fcbeaap-105},
    {0x1.c493853e7ddcbp-104, 0x1.891a4d042dafp-105},
    {0x1.d181cae131004p-104, 0x1.9455910b9cb4p-105},
    {0x1.de701083e423dp-104, 0x1.9f90d5130bb9p-105},
    {0x1.eb5e562697476p-104, 0x1.aacc191a7abep-105},
    {0x1.f84c9bc94a6afp-104, 0x1.b6075d21e9c3p-105},
    {0x1.029d70b5fec74p-103, 0x1.c142a12958c8p-105},
    {0x1.0f8bb658b1eadp-103, 0x1.d7b9293836d2p-105},
    {0x1.1c79fbfb650e6p-103, 0x1.ee2fb14714dcp-105},
    {0x1.2968419e1831fp-103, 0x1.02531caaf973p-104},
    {0x1.36568740cb558p-103, 0x1.0d8e60b26878p-104},
    {0x1.4344cce37e791p-103, 0x1.18c9a4b9d77dp-104},
    {0x1.50331286319cap-103, 0x1.2404e8c14682p-104},
    {0x1.5d215828e4c03p-103, 0x1.2f402cc8b587p-104},
    {0x1.dd9e9781a5e79p-101, 0x1.9edae2fbcd64bp-102},
    {0x1.b6024d974e40ap-100, 0x1.7c7324ce42017p-101},
    {0x1.c2f0933a01643p-100, 0x1.87ae68d5b1067p-101},
    {0x1.34b3953c4eddp-99, 0x1.0c227c83ebcf7p-100},
    {0x1.0d174b51f7361p-98, 0x1.d3757cacc0d85p-100},
    {0x1.3310a337d2648p-98, 0x1.0ab698556f26ep-99},
    {0x1.0aa2e04b3c815p-97, 0x1.cf31d0214adeap-99},
    {0x1.efcb4cb07acf9p-96, 0x1.aea42d608df93p-97},
    {0x1.5642fb14919d5p-95, 0x1.2948e135d665cp-96},
    {0x1.768203eab926p-95, 0x1.454b1c9230d28p-96},
    {0x1.6043faa94344ep-93, 0x1.31f955ab16c63p-94},
    {0x1.6777f13c42c2bp-93, 0x1.383afb0fd17d3p-94},
    {0x1.7d7e8dd9fb4d8p-91, 0x1.4b5c9dde66d79p-92},
    {0x1.460fc5b3d0553p-90, 0x1.1b36a9020b0f1p-91},
    {0x1.5f9fc4ef6b8f9p-90, 0x1.316ab429dac7ap-91},
    {0x1.ea9ddc27a6265p-90, 0x1.aa24ea4121da8p-91},
    {0x1.1af022242c32dp-89, 0x1.eb8380da46f64p-91},
    {0x1.20ad75b02af2ap-89, 0x1.f57bfc118dbfcp-91},
    {0x1.288c9f4083265p-89, 0x1.0194571138c27p-90},
    {0x1.eca766ae1c14fp-89, 0x1.abe9eb7f5f89ap-90},
    {0x1.96d8e16c2415bp-88, 0x1.61620ae7d3c49p-89},
    {0x1.c51a0b95b13b9p-88, 0x1.898f25c7677fdp-89},
    {0x1.d6e305234d12cp-88, 0x1.9901cfae3e0cp-89},
    {0x1.41aa857242e05p-84, 0x1.1765479305154p-85},
    {0x1.8c5b2f454a901p-84, 0x1.584546dad1c55p-85},
    {0x1.e25876de33b0cp-84, 0x1.a2f5c4c06a6edp-85},
    {0x1.a290d06c2930dp-83, 0x1.6b8fc076bdcf1p-84},
    {0x1.9b62f82306ed5p-81, 0x1.65536b8802cbep-82},
    {0x1.86c621927ea1dp-80, 0x1.536c044472357p-81},
    {0x1.6a2944b547b54p-74, 0x1.3a91b8c2d5df1p-75},
    {0x1.06dfe1920e02ap-73, 0x1.c8a8eb58ffa81p-75},
    {0x1.28c0939e3c4a1p-71, 0x1.01c1779e5526ep-72},
    {0x1.402e17b9ceb1dp-71, 0x1.161ad7f017b99p-72},
    {0x1.42cf56d59d69ep-71, 0x1.18639e356e22ep-72},
    {0x1.b515c3d97841fp-70, 0x1.7ba5b07e7b617p-71},
    {0x1.29f6fd191d3f8p-69, 0x1.02cf16751917ep-70},
    {0x1.ff117ee23d90cp-69, 0x1.bbe887c8df889p-70},
    {0x1.1402bd3997de6p-68, 0x1.df7acd53ec371p-70},
    {0x1.d796634d9b938p-68, 0x1.999d9bb1352d1p-69},
    {0x1.2047f49b947dbp-67, 0x1.f4cba75c44271p-69},
    {0x1.4c3d24ca6e1fep-67, 0x1.209438d2d2728p-68},
    {0x1.4da1834c5c59ep-67, 0x1.21c9c2a2b2855p-68},
    {0x1.90f3223325299p-67, 0x1.5c42b16bae289p-68},
    {0x1.a27d1e6b578afp-66, 0x1.6b7ea50916814p-67},
    {0x1.69f4417ded8fcp-65, 0x1.3a63acf3d3444p-66},
    {0x1.6dade523142d8p-65, 0x1.3d9fff014ee3ap-66},
    {0x1.c1e176fc7fa1p-65, 0x1.86c2ed13dd9a1p-66},
    {0x1.e0ab9adf159p-65, 0x1.a181441c0585dp-66},
    {0x1.08c9f0d7b2f8dp-64, 0x1.cbfc3d7a26e73p-66},
    {0x1.1b928fa39c09ep-64, 0x1.ec9dab50150a6p-66},
    {0x1.1f32ca3aec2c6p-64, 0x1.f2ea2afb5995ep-66},
    {0x1.bb0b2a5a28b2fp-64, 0x1.80d2a2a882e81p-65},
    {0x1.22f984b11eaa8p-63, 0x1.f9798c0d41dacp-65},
    {0x1.7a3aacf87970ep-62, 0x1.488694f679cf4p-63},
    {0x1.d512e0d50be54p-61, 0x1.976ea9aca8476p-62},
    {0x1.16e41f9dc2092p-60, 0x1.e47bc515bd69ep-62},
    {0x1.52288c851532ap-60, 0x1.25b87c7717e1ap-61},
    {0x1.d3ddc26e4823ap-60, 0x1.96622a681e038p-61},
    {0x1.187b0ff7df629p-58, 0x1.e73eb1f1b7f2ap-60},
    {0x1.3808b59a5e23ep-58, 0x1.0f0780aa05b7ep-59},
    {0x1.4d7591583c99bp-58, 0x1.21a3970ccc033p-59},
    {0x1.ab8a597feec6fp-58, 0x1.735b5da21781bp-59},
    {0x1.9343d1b20205ap-57, 0x1.5e457e3faf9e8p-58},
    {0x1.12e914b171384p-55, 0x1.dd9182ed37a3dp-57},
    {0x1.de0a6b501ba08p-55, 0x1.9f388b5863db3p-56},
    {0x1.5e371fd65b154p-54, 0x1.3031739a34e4dp-55},
    {0x1.77844e5a866d3p-54, 0x1.462b75ca341d5p-55},
    {0x1.7ad01bd0d00fep-50, 0x1.490860b2f4209p-51},
    {0x1.9cdf2a6fc8274p-50, 0x1.669da78e39647p-51},
    {0x1.5ece71bdb9a78p-48, 0x1.30b4e2eb07466p-49},
    {0x1.f7c0c8ddfa561p-47, 0x1.b58dea0f8a453p-48},
    {0x1.6004d91a3c121p-46, 0x1.31c27feab2a1dp-47},
    {0x1.8f2da848b6c4p-46, 0x1.5ab8cf0041761p-47},
    {0x1.ccb172a1af7dbp-44, 0x1.90272a6232b1ep-45},
    {0x1.058f40c3fe6f9p-43, 0x1.c66022e77da7dp-45},
    {0x1.9c0f689015cedp-42, 0x1.65e932e47d071p-43},
    {0x1.c835a792c7d28p-42, 0x1.8c42347ead386p-43},
    {0x1.541734d99aa3dp-41, 0x1.276623e8de422p-42},
    {0x1.c264bef2a8545p-37, 0x1.8734f494fa237p-38},
    {0x1.4805e5b5690fdp-36, 0x1.1ceacce9b9924p-37},
    {0x1.5c60678e63895p-35, 0x1.2e9896e9ce462p-36},
    {0x1.04413d5862a05p-34, 0x1.c41be54cd43eap-36},
    {0x1.dab201bddf297p-34, 0x1.9c50ac88ddb55p-35},
    {0x1.25098367f0da8p-33, 0x1.fd0ec4ae92889p-35},
    {0x1.6c7773f94688ap-31, 0x1.3c92597c554fep-32},
    {0x1.ffc754901a621p-31, 0x1.bc86784f05a4fp-32},
    {0x1.1dcf2156216fcp-30, 0x1.f08052d61f7d1p-32},
    {0x1.3c517a92b1982p-30, 0x1.12c024f4f68afp-31},
    {0x1.d6daed91d1de7p-30, 0x1.98fac850facd7p-31},
    {0x1.f373a6544d592p-26, 0x1.b1d17aa3266dep-27},
    {0x1.26cde5952cfc1p-22, 0x1.00104f7aced6ap-23},
    {0x1.d72ff21133ed1p-22, 0x1.99449adb4bd85p-23},
    {0x1.7bdeb19dea952p-18, 0x1.49f32a7bd55bap-19},
    {0x1.0ff50ee40d0fp-16, 0x1.d86f2f7060878p-18},
    {0x1.88520d43748bap-16, 0x1.54c2e3983454dp-17},
    {0x1.15fb6a34fb176p-15, 0x1.e2e5771d5ab1fp-17},
    {0x1.44ae28262d96cp-14, 0x1.1a00b833c901cp-15},
    {0x1.01124eefc36a4p-12, 0x1.be86340eed107p-14},
    {0x1.41b5c94e3c3dep-12, 0x1.176417c3f87acp-13},
    {0x1.a2f556d47c36fp-12, 0x1.6bd475fa8de88p-13},
    {0x1.ce41567084f6cp-11, 0x1.915538385e79dp-12},
    {0x1.fb9e9f5433b54p-11, 0x1.b8b311b108dd4p-12},
    {0x1.59b3702e04487p-10, 0x1.2c12fe8018b6p-11},
    {0x1.c3ccf9f41e6aap-10, 0x1.88175ff51d9bap-11},
    {0x1.6850da9c8ae72p-9, 0x1.388977240bca5p-10},
    {0x1.df0174571328cp-9, 0x1.9f4cf7329872bp-10},
    {0x1.ba50183dc2a68p-8, 0x1.7ee5bc8b4213bp-9},
    {0x1.69039159cbd42p-7, 0x1.37dba262ebba9p-8},
    {0x1.b9ecff4fdee34p-6, 0x1.7ac42a86a5f66p-7},
  };
static const int8_t exceptions_rnd[EXCEPTIONS] = {
    1, /* -0x1.e0648eff3dad8p-6 */
    -1, /* -0x1.8dd157e27ade5p-6 */
    1, /* -0x1.739b846071578p-6 */
    1, /* -0x1.a3d1b9ce5737fp-7 */
    1, /* -0x1.6755766dea6c8p-8 */
    -1, /* -0x1.602d6acef178ap-8 */
    1, /* -0x1.26ffe1529fb7p-8 */
    -1, /* -0x1.be50b576d2dcbp-9 */
    -1, /* -0x1.1279455abcf43p-9 */
    -1, /* -0x1.0828a8bbb67a6p-9 */
    1, /* -0x1.e5bc53490e628p-11 */
    -1, /* -0x1.817b775a8e65cp-11 */
    1, /* -0x1.cf968a9b81e41p-12 */
    -1, /* -0x1.13f6403cf8b55p-12 */
    1, /* -0x1.f5ec2799a555p-18 */
    1, /* -0x1.b8e37862e8ea5p-20 */
    1, /* -0x1.afbdf68019c4p-21 */
    -1, /* -0x1.a7ab033c58b46p-23 */
    1, /* -0x1.8c14a6409f43bp-23 */
    -1, /* -0x1.9c74c219ae88fp-30 */
    -1, /* -0x1.cb033247d6af5p-31 */
    1, /* -0x1.8da6667882dcep-32 */
    -1, /* -0x1.8358a7e7e36dfp-34 */
    -1, /* -0x1.07a54aa3098b2p-36 */
    1, /* -0x1.8b3f17bc52fep-37 */
    1, /* -0x1.066ca101bf96fp-38 */
    -1, /* -0x1.be1f254f3131fp-39 */
    1, /* -0x1.109e8e5411922p-39 */
    -1, /* -0x1.b169f3f07565bp-40 */
    1, /* -0x1.ebee0a568ac68p-41 */
    1, /* -0x1.4d089375ad275p-41 */
    -1, /* -0x1.7ba91f831d839p-45 */
    1, /* -0x1.ed9433cd12dfep-47 */
    1, /* -0x1.011108c573ad1p-49 */
    -1, /* -0x1.b806fbada31eep-50 */
    -1, /* -0x1.9dd7565d325e4p-52 */
    1, /* -0x1.84f828be9e9f3p-55 */
    1, /* -0x1.fa19033240b03p-57 */
    1, /* -0x1.5b97ae0171027p-57 */
    1, /* -0x1.50060db4d7c31p-57 */
    1, /* -0x1.191ff3e3d56e7p-62 */
    1, /* -0x1.01f6630b953f2p-62 */
    1, /* -0x1.1d16bd3e005a9p-63 */
    1, /* -0x1.f4e57e1eadaf2p-64 */
    1, /* -0x1.eda036363f823p-64 */
    1, /* -0x1.3ef3ee3f1171dp-66 */
    1, /* -0x1.b356682c47d4cp-67 */
    1, /* -0x1.2684e330dcfcfp-67 */
    1, /* -0x1.17133b2d0251p-67 */
    1, /* -0x1.1223eea2a0873p-68 */
    1, /* -0x1.ede211e678806p-69 */
    1, /* -0x1.d148e85c20981p-70 */
    1, /* -0x1.7829cd77bf34ep-71 */
    1, /* -0x1.041ec1d294952p-75 */
    -1, /* -0x1.bd58bcace9d65p-79 */
    1, /* -0x1.b731f404a5e5fp-79 */
    1, /* -0x1.a55ec4a3bc202p-79 */
    -1, /* -0x1.97f1eac52ac07p-79 */
    1, /* -0x1.9110b1168be32p-79 */
    1, /* -0x1.f1d2145ff142ep-84 */
    1, /* -0x1.1f5de53a357d4p-84 */
    1, /* -0x1.06dca67f311a5p-86 */
    -1, /* -0x1.e6f45d7354495p-87 */
    1, /* -0x1.13c9a89fe407fp-87 */
    1, /* -0x1.88fb6cbee3cf8p-89 */
    1, /* -0x1.0cd7d95208b86p-96 */
    -1, /* -0x1.2cae25a71122fp-98 */
    1, /* -0x1.1d4b74fda34aap-100 */
    1, /* 0x1.2968419e1831fp-105 */
    1, /* 0x1.36568740cb558p-105 */
    1, /* 0x1.4344cce37e791p-105 */
    1, /* 0x1.50331286319cap-105 */
    1, /* 0x1.5d215828e4c03p-105 */
    1, /* 0x1.6a0f9dcb97e3cp-105 */
    1, /* 0x1.76fde36e4b075p-105 */
    1, /* 0x1.83ec2910fe2aep-105 */
    1, /* 0x1.90da6eb3b14e7p-105 */
    1, /* 0x1.9dc8b4566472p-105 */
    1, /* 0x1.aab6f9f917959p-105 */
    1, /* 0x1.b7a53f9bcab92p-105 */
    1, /* 0x1.c493853e7ddcbp-105 */
    1, /* 0x1.d181cae131004p-105 */
    1, /* 0x1.de701083e423dp-105 */
    1, /* 0x1.eb5e562697476p-105 */
    1, /* 0x1.f84c9bc94a6afp-105 */
    1, /* 0x1.029d70b5fec74p-104 */
    1, /* 0x1.0f8bb658b1eadp-104 */
    1, /* 0x1.1c79fbfb650e6p-104 */
    1, /* 0x1.2968419e1831fp-104 */
    1, /* 0x1.36568740cb558p-104 */
    1, /* 0x1.4344cce37e791p-104 */
    1, /* 0x1.50331286319cap-104 */
    1, /* 0x1.5d215828e4c03p-104 */
    1, /* 0x1.6a0f9dcb97e3cp-104 */
    1, /* 0x1.76fde36e4b075p-104 */
    1, /* 0x1.83ec2910fe2aep-104 */
    1, /* 0x1.90da6eb3b14e7p-104 */
    1, /* 0x1.9dc8b4566472p-104 */
    1, /* 0x1.aab6f9f917959p-104 */
    1, /* 0x1.b7a53f9bcab92p-104 */
    1, /* 0x1.c493853e7ddcbp-104 */
    1, /* 0x1.d181cae131004p-104 */
    1, /* 0x1.de701083e423dp-104 */
    1, /* 0x1.eb5e562697476p-104 */
    1, /* 0x1.f84c9bc94a6afp-104 */
    1, /* 0x1.029d70b5fec74p-103 */
    1, /* 0x1.0f8bb658b1eadp-103 */
    1, /* 0x1.1c79fbfb650e6p-103 */
    1, /* 0x1.2968419e1831fp-103 */
    1, /* 0x1.36568740cb558p-103 */
    -1, /* 0x1.4344cce37e791p-103 */
    -1, /* 0x1.50331286319cap-103 */
    -1, /* 0x1.5d215828e4c03p-103 */
    1, /* 0x1.dd9e9781a5e79p-101 */
    1, /* 0x1.b6024d974e40ap-100 */
    1, /* 0x1.c2f0933a01643p-100 */
    1, /* 0x1.34b3953c4eddp-99 */
    1, /* 0x1.0d174b51f7361p-98 */
    1, /* 0x1.3310a337d2648p-98 */
    1, /* 0x1.0aa2e04b3c815p-97 */
    1, /* 0x1.efcb4cb07acf9p-96 */
    -1, /* 0x1.5642fb14919d5p-95 */
    1, /* 0x1.768203eab926p-95 */
    1, /* 0x1.6043faa94344ep-93 */
    1, /* 0x1.6777f13c42c2bp-93 */
    1, /* 0x1.7d7e8dd9fb4d8p-91 */
    1, /* 0x1.460fc5b3d0553p-90 */
    1, /* 0x1.5f9fc4ef6b8f9p-90 */
    1, /* 0x1.ea9ddc27a6265p-90 */
    1, /* 0x1.1af022242c32dp-89 */
    1, /* 0x1.20ad75b02af2ap-89 */
    1, /* 0x1.288c9f4083265p-89 */
    1, /* 0x1.eca766ae1c14fp-89 */
    1, /* 0x1.96d8e16c2415bp-88 */
    -1, /* 0x1.c51a0b95b13b9p-88 */
    1, /* 0x1.d6e305234d12cp-88 */
    1, /* 0x1.41aa857242e05p-84 */
    1, /* 0x1.8c5b2f454a901p-84 */
    -1, /* 0x1.e25876de33b0cp-84 */
    1, /* 0x1.a290d06c2930dp-83 */
    1, /* 0x1.9b62f82306ed5p-81 */
    -1, /* 0x1.86c621927ea1dp-80 */
    -1, /* 0x1.6a2944b547b54p-74 */
    1, /* 0x1.06dfe1920e02ap-73 */
    1, /* 0x1.28c0939e3c4a1p-71 */
    1, /* 0x1.402e17b9ceb1dp-71 */
    1, /* 0x1.42cf56d59d69ep-71 */
    1, /* 0x1.b515c3d97841fp-70 */
    1, /* 0x1.29f6fd191d3f8p-69 */
    1, /* 0x1.ff117ee23d90cp-69 */
    1, /* 0x1.1402bd3997de6p-68 */
    -1, /* 0x1.d796634d9b938p-68 */
    1, /* 0x1.2047f49b947dbp-67 */
    1, /* 0x1.4c3d24ca6e1fep-67 */
    -1, /* 0x1.4da1834c5c59ep-67 */
    1, /* 0x1.90f3223325299p-67 */
    1, /* 0x1.a27d1e6b578afp-66 */
    1, /* 0x1.69f4417ded8fcp-65 */
    1, /* 0x1.6dade523142d8p-65 */
    1, /* 0x1.c1e176fc7fa1p-65 */
    1, /* 0x1.e0ab9adf159p-65 */
    1, /* 0x1.08c9f0d7b2f8dp-64 */
    -1, /* 0x1.1b928fa39c09ep-64 */
    -1, /* 0x1.1f32ca3aec2c6p-64 */
    1, /* 0x1.bb0b2a5a28b2fp-64 */
    -1, /* 0x1.22f984b11eaa8p-63 */
    -1, /* 0x1.7a3aacf87970ep-62 */
    -1, /* 0x1.d512e0d50be54p-61 */
    1, /* 0x1.16e41f9dc2092p-60 */
    1, /* 0x1.52288c851532ap-60 */
    1, /* 0x1.d3ddc26e4823ap-60 */
    1, /* 0x1.187b0ff7df629p-58 */
    -1, /* 0x1.3808b59a5e23ep-58 */
    -1, /* 0x1.4d7591583c99bp-58 */
    1, /* 0x1.ab8a597feec6fp-58 */
    1, /* 0x1.9343d1b20205ap-57 */
    -1, /* 0x1.12e914b171384p-55 */
    -1, /* 0x1.de0a6b501ba08p-55 */
    1, /* 0x1.5e371fd65b154p-54 */
    -1, /* 0x1.77844e5a866d3p-54 */
    1, /* 0x1.7ad01bd0d00fep-50 */
    -1, /* 0x1.9cdf2a6fc8274p-50 */
    1, /* 0x1.5ece71bdb9a78p-48 */
    -1, /* 0x1.f7c0c8ddfa561p-47 */
    1, /* 0x1.6004d91a3c121p-46 */
    -1, /* 0x1.8f2da848b6c4p-46 */
    1, /* 0x1.ccb172a1af7dbp-44 */
    1, /* 0x1.058f40c3fe6f9p-43 */
    1, /* 0x1.9c0f689015cedp-42 */
    1, /* 0x1.c835a792c7d28p-42 */
    1, /* 0x1.541734d99aa3dp-41 */
    1, /* 0x1.c264bef2a8545p-37 */
    -1, /* 0x1.4805e5b5690fdp-36 */
    1, /* 0x1.5c60678e63895p-35 */
    -1, /* 0x1.04413d5862a05p-34 */
    1, /* 0x1.dab201bddf297p-34 */
    -1, /* 0x1.25098367f0da8p-33 */
    1, /* 0x1.6c7773f94688ap-31 */
    1, /* 0x1.ffc754901a621p-31 */
    1, /* 0x1.1dcf2156216fcp-30 */
    1, /* 0x1.3c517a92b1982p-30 */
    -1, /* 0x1.d6daed91d1de7p-30 */
    -1, /* 0x1.f373a6544d592p-26 */
    -1, /* 0x1.26cde5952cfc1p-22 */
    1, /* 0x1.d72ff21133ed1p-22 */
    -1, /* 0x1.7bdeb19dea952p-18 */
    -1, /* 0x1.0ff50ee40d0fp-16 */
    1, /* 0x1.88520d43748bap-16 */
    1, /* 0x1.15fb6a34fb176p-15 */
    1, /* 0x1.44ae28262d96cp-14 */
    -1, /* 0x1.01124eefc36a4p-12 */
    -1, /* 0x1.41b5c94e3c3dep-12 */
    -1, /* 0x1.a2f556d47c36fp-12 */
    -1, /* 0x1.ce41567084f6cp-11 */
    -1, /* 0x1.fb9e9f5433b54p-11 */
    1, /* 0x1.59b3702e04487p-10 */
    -1, /* 0x1.c3ccf9f41e6aap-10 */
    -1, /* 0x1.6850da9c8ae72p-9 */
    1, /* 0x1.df0174571328cp-9 */
    -1, /* 0x1.ba50183dc2a68p-8 */
    -1, /* 0x1.69039159cbd42p-7 */
    1, /* 0x1.b9ecff4fdee34p-6 */
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

  /* for degree 11 or more, ulp(c[d]*x^d) < 2^-105.7*|log10p1(x)|
     where c[d] is the degree-d coefficient of Pacc, thus we can compute
     with a double only, and even with degree 10 (this does not increase
     the number of exceptional cases) */

  h = __builtin_fma (Pacc[24], x, Pacc[23]);    // degree 16
  // degrees 15 down to 10
  for (int i = 15; i >= 10; i--)
    h = __builtin_fma (h, x, Pacc[i+7]);
  // degree 9
  a_mul (&h, &l, x, h);
  fast_two_sum (&h, &t, Pacc[9+7], h);
  l += t;
  // degrees 8 down to 1
  for (int i = 8; i >= 1; i--)
    {
      s_mul (&h, &l, x, h, l);
      fast_two_sum (&h, &t, Pacc[2*i-2], h);
      l += t + Pacc[2*i-1];
    }
  /* final multiplication by x */
  s_mul (&h, &l, x, h, l);
  return h + l;
}

/* accurate path, using Tom Hubrecht's code below */
static double
cr_log10p1_accurate (double x)
{
  dint64_t X, Y, C;
  double ax = __builtin_fabs (x);

  if (ax < 0x1p-5)
    return (ax < 0x1p-900) ? cr_log10p1_accurate_tiny (x)
      : cr_log10p1_accurate_small (x);

#define EXCEPTIONS 13
  static const double T[EXCEPTIONS][3] = {
    {0x1.2p+3, 0x1p+0, 0x0p+0},
    {0x1.e847ep+19, 0x1.8p+2, 0x0p+0},
    {0x1.8cp+6, 0x1p+1, 0x0p+0},
    {0x1.d1a94a1ffep+39, 0x1.8p+3, 0x0p+0},
    {0x1.312cfep+23, 0x1.cp+2, 0x0p+0},
    {0x1.869fp+16, 0x1.4p+2, 0x0p+0},
    {0x1.2309ce53ffep+43, 0x1.ap+3, 0x0p+0},
    {0x1.7d783fcp+26, 0x1p+3, 0x0p+0},
    {0x1.3878p+13, 0x1p+2, 0x0p+0},
    {0x1.d01b2ef68a124p+763, 0x1.cbe37694f4d1p+7, -0x1.812916cee50fap-108},
    {0x1.c6bf52633fff8p+49, 0x1.ep+3, 0x0p+0},
    {0x1.f38p+9, 0x1.8p+1, 0x0p+0},
    {0x1.2a05f1ff8p+33, 0x1.4p+3, 0x0p+0},
  };
  for (int i = 0; i < EXCEPTIONS; i++)
    if (x == T[i][0])
      return T[i][1] + T[i][2];
#undef EXCEPTIONS

  /* (xh,xl) <- 1+x */
  double xh, xl;
  if (x > 1.0)
    fast_two_sum (&xh, &xl, x, 1.0);
  else
    fast_two_sum (&xh, &xl, 1.0, x);

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

  /* multiply by 1/log(10) */
  mul_dint_126 (&Y, &Y, &LOG10_INV);

  return dint_tod_not_subnormal (&Y);
}

/* Given x > -1, put in (h,l) a double-double approximation of log2(1+x),
   and return a bound err on the maximal absolute error so that:
   |h + l - log2(1+x)| < err.
   We have x = m*2^e with 1 <= m < 2 (m = v.f) and -1074 <= e <= 1023.
   This routine is adapted from cr_log1p_fast.
*/
static double
cr_log10p1_fast (double *h, double *l, double x, int e, d64u64 v)
{
  if (e < -5) /* e <= -6 thus |x| < 2^-5 */
  {
    double lo;
    if (e <= -968)
    {
      /* then |x| might be as small as 2^-968, thus h=x/log(10) might in the
         binade [2^-970,2^-969), with ulp(h) = 2^-1022, and if |l| < ulp(h),
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
    d_mul (h, l, *h, *l, INVLOG10H, INVLOG10L);
    /* the d_mul() call decomposes into:
       a_mul (h_out, l1, h, INVLOG10H)
       l2 = __builtin_fma (h, INVLOG10L, l1)
       l_out = __builtin_fma (l, INVLOG10H, l2)
       we have |l1| <= ulp(h_out)
       since |INVLOG10L/INVLOG10H| < 2^-55, then |h*INVLOG10L| <= 2^-55*|h_out|
       and since |x| < 2^53*ulp(x): |h*INVLOG10L| <= ulp(h_out)/4
       thus |l2| <= 5/4*ulp(h_out).
       Now since |l/h| < 2^-50.96, |l*INVLOG10H| < 2^-50.96*|h*INVLOG10H|
       < 2^-50.96*(1+2^-52)*|h_out| < 2^-50.95*|h_out| < 4.15*ulp(h_out),
       thus |l_out| < o(4.15*ulp(h_out)+5/4*ulp(h_out)) < 5.5*ulp(h_out).
       The rounding errors are bounded by ulp(l2)+ulp(l_out)
       <= ulp(5/4*ulp(h_out)) + ulp(5.5*ulp(h_out))
       <= 2^-52*(5/4*ulp(h_out)+5.5*ulp(h_out)) [using ulp(x) <= 2^-52*|x|]
       <= 2^-49.2*ulp(h_out)
       We also have to take into account the ignored term l*INVLOG10L:
       |l*INVLOG10L| < 2^-50.96*|h|*2^-55.97*|INVLOG10H|
                     < 2^-106.93*(1+2^-52)*|h_out|
                     < 2^-106.92*|h_out|
                     < 2^-51.92*ulp(h_out) [using |x| < 2^53*ulp(x)]
      and the approximation error in INVLOG10H+INVLOG10L:
      |INVLOG10H + INVLOG10L - 1/log(10)| < 2^-109.84/log(10)
      The total error of d_mul() is thus bounded by:
      (2^-49.2+2^-51.92)*ulp(h_out) < 2^-48.99*ulp(h_out) < 2^-100.99*|h_out|,
      using again ulp(x) <= 2^-52*|x|.

      The relative error is thus bounded by
      (1+2^-61.14)*(1+2^-100.99)*(1+2^-109.84)-1 < 2^-61.13 */
    return 0x1.d4p-62 * *h; /* 2^-61.13 < 0x1.d4p-62 */
  }

  /* (xh,xl) <- 1+x */
  double xh, xl;
  if (x > 1.0) {
    if (x < 0x1p53)
      fast_two_sum (&xh, &xl, x, 1.0);
    else // avoid spurious overflow in x + 1.0
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
  if (__builtin_expect (xh <= 0x1p1022, 1))
    c = xl / xh;
  else
    c = 0; // avoid spurious underflow in xl / xh
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
  d_mul (h, l, *h, *l, INVLOG10H, INVLOG10L);
  /* the d_mul() call decomposes into:
     a_mul (h_out, l1, h, INVLOG10H)
     l2 = __builtin_fma (h, INVLOG10L, l1)
     l_out = __builtin_fma (l, INVLOG10H, l2)
     We have three errors:
     * the rounding error in l2 = __builtin_fma (h, INVLOG10L, l1)
     * the rounding error in l_out = __builtin_fma (l, INVLOG10H, l2)
     * the ignored term l * INVLOG10L
     We have |h| < 745 thus |h*INVLOG10H| < 324 thus |h_out| <= 324
     and |l1| <= ulp(h_out) <= 2^-44.
     Then |h*INVLOG10L+l1| <= 745*INVLOG2L+2^-44 < 2^-43.6
     thus |l2| < 2^-43.6*(1+2^-52) < 2^-43.5
     and the first rounding error is bounded by ulp(2^-43.5) = 2^-96.
     Now |l*INVLOG10H+l2| < 2^-18.68*INVLOG10H+2^-43.5 < 2^-19.8
     thus |l_out| < 2^-19.8*(1+2^-52) < 2^-19.7
     and the second rounding error is bounded by ulp(2^-19.7) = 2^-72.
     The ignored term is bounded by |l*INVLOG10L| < 2^-18.68*INVLOG10L
     < 2^-75.0.
     Thus the absolute error from d_mul() is bounded by:
     2^-96 + 2^-72 + 2^-75.0 < 2^-71.83.

     Adding to the maximal absolute error of 2^-68.02 before d_mul(),
     we get 2^-68.02 + 2^-71.83 < 2^-67.92.
  */

  return 0x1.0ap-68; /* 2^-67.92 < 0x1.0ap-68 */
}

double
__log10p1 (double x)
{
  d64u64 v = {.f = x};
  int e = ((v.u >> 52) & 0x7ff) - 0x3ff;
  if (__builtin_expect (e == 0x400 || x == 0 || x <= -1.0, 0))
    /* case NaN/Inf, +/-0 or x <= -1 */
  {
    static const d64u64 minf = {.u = 0xfffull << 52};
    if (e == 0x400 && x != minf.f){ /* NaN or + Inf*/
      return x + x;
    }
    if (x <= -1.0) /* we use the fact that NaN < -1 is false */
    {
      /* log10p(x<-1) is NaN, log2p(-1) is -Inf and raises DivByZero */
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

  /* check x=10^n-1 for 1 <= n <= 15, where log10p1(x) is exact,
     and we shouldn't raise the inexact flag */
  if (__builtin_expect (3 <= e && e <= 49, 1)) {
    /* T[e] is zero if there is no value of the form 10^n-1 in the range
       [2^e, 2^(e+1)), otherwise it is this (unique) value. */
    static const double T[] = {
      0x0p+0, 0x0p+0, 0x0p+0, 0x1.2p+3, 0x0p+0, 0x0p+0, 0x1.8cp+6, 0x0p+0, 0x0p+0, 0x1.f38p+9, 0x0p+0, 0x0p+0, 0x0p+0, 0x1.3878p+13, 0x0p+0, 0x0p+0, 0x1.869fp+16, 0x0p+0, 0x0p+0, 0x1.e847ep+19, 0x0p+0, 0x0p+0, 0x0p+0, 0x1.312cfep+23, 0x0p+0, 0x0p+0, 0x1.7d783fcp+26, 0x0p+0, 0x0p+0, 0x1.dcd64ff8p+29, 0x0p+0, 0x0p+0, 0x0p+0, 0x1.2a05f1ff8p+33, 0x0p+0, 0x0p+0, 0x1.74876e7ffp+36, 0x0p+0, 0x0p+0, 0x1.d1a94a1ffep+39, 0x0p+0, 0x0p+0, 0x0p+0, 0x1.2309ce53ffep+43, 0x0p+0, 0x0p+0, 0x1.6bcc41e8fffcp+46, 0x0p+0, 0x0p+0, 0x1.c6bf52633fff8p+49};
    // U[e] is the integer n such that T[e] = 10^n-1 when T[e] is not zero
    static const int U[] = {
      0, 0, 0, 1, 0, 0, 2, 0, 0, 3, 0, 0, 0, 4, 0, 0, 5, 0, 0, 6, 0, 0, 0, 7, 0, 0, 8, 0, 0, 9, 0, 0, 0, 10, 0, 0, 11, 0, 0, 12, 0, 0, 0, 13, 0, 0, 14, 0, 0, 15};
    if (x == T[e])
      return U[e];
  }

  /* now x > -1 */
  /* normalize v in [1,2) */
  v.u = (0x3ffull << 52) | (v.u & 0xfffffffffffff);
  /* now x = m*2^e with 1 <= m < 2 (m = v.f) and -1074 <= e <= 1023 */
  double h, l, err;
  err = cr_log10p1_fast (&h, &l, x, e, v);

  double left = h + (l - err), right = h + (l + err);
  if (__builtin_expect (left == right, 1))
    return left;

  return cr_log10p1_accurate (x);
}
libm_alias_double (__log10p1, log10p1)

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
