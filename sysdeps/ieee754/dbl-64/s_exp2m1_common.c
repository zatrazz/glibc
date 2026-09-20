/* Common data for the binary64 exp2m1 and exp10m1 implementations.

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

#include <math.h>
#include <stdint.h>
#include "math_config.h"
#include "s_exp2m1_common.h"

static const double Q_1[] = {
  0x1p0,		/* degree 0 */
  0x1p0,		/* degree 1 */
  0x1p-1,		/* degree 2 */
  0x1.5555555995d37p-3, /* degree 3 */
  0x1.55555558489dcp-5	/* degree 4 */
};

// Approximation for the fast path of exp(z) for z=zh+zl,
// with |z| < 0.000130273 < 2^-12.88 and |zl| < 2^-42.6
// (assuming x^y does not overflow or underflow)
static inline void
q_1 (double *hi, double *lo, double zh, double zl)
{
  double z = zh + zl;
  double q = fma (Q_1[4], zh, Q_1[3]);

  q = fma (q, z, Q_1[2]);

  fast_two_sum (hi, lo, Q_1[1], q * z);

  d_mul (hi, lo, zh, zl, *hi, *lo);

  fast_sum (hi, lo, Q_1[0], *hi, *lo);
}

/* For 0 <= i < 64, T1[i] = (h,l) such that h+l is the best double-double
   approximation of 2^(i/64). The approximation error is bounded as follows:
   |h + l - 2^(i/64)| < 2^-107. */

const double __exp2m1_t1[64][2] = {
  { 0x1p+0, 0x0p+0 },
  { 0x1.02c9a3e778061p+0, -0x1.19083535b085dp-56 },
  { 0x1.059b0d3158574p+0, 0x1.d73e2a475b465p-55 },
  { 0x1.0874518759bc8p+0, 0x1.186be4bb284ffp-57 },
  { 0x1.0b5586cf9890fp+0, 0x1.8a62e4adc610bp-54 },
  { 0x1.0e3ec32d3d1a2p+0, 0x1.03a1727c57b53p-59 },
  { 0x1.11301d0125b51p+0, -0x1.6c51039449b3ap-54 },
  { 0x1.1429aaea92dep+0, -0x1.32fbf9af1369ep-54 },
  { 0x1.172b83c7d517bp+0, -0x1.19041b9d78a76p-55 },
  { 0x1.1a35beb6fcb75p+0, 0x1.e5b4c7b4968e4p-55 },
  { 0x1.1d4873168b9aap+0, 0x1.e016e00a2643cp-54 },
  { 0x1.2063b88628cd6p+0, 0x1.dc775814a8495p-55 },
  { 0x1.2387a6e756238p+0, 0x1.9b07eb6c70573p-54 },
  { 0x1.26b4565e27cddp+0, 0x1.2bd339940e9d9p-55 },
  { 0x1.29e9df51fdee1p+0, 0x1.612e8afad1255p-55 },
  { 0x1.2d285a6e4030bp+0, 0x1.0024754db41d5p-54 },
  { 0x1.306fe0a31b715p+0, 0x1.6f46ad23182e4p-55 },
  { 0x1.33c08b26416ffp+0, 0x1.32721843659a6p-54 },
  { 0x1.371a7373aa9cbp+0, -0x1.63aeabf42eae2p-54 },
  { 0x1.3a7db34e59ff7p+0, -0x1.5e436d661f5e3p-56 },
  { 0x1.3dea64c123422p+0, 0x1.ada0911f09ebcp-55 },
  { 0x1.4160a21f72e2ap+0, -0x1.ef3691c309278p-58 },
  { 0x1.44e086061892dp+0, 0x1.89b7a04ef80dp-59 },
  { 0x1.486a2b5c13cdp+0, 0x1.3c1a3b69062fp-56 },
  { 0x1.4bfdad5362a27p+0, 0x1.d4397afec42e2p-56 },
  { 0x1.4f9b2769d2ca7p+0, -0x1.4b309d25957e3p-54 },
  { 0x1.5342b569d4f82p+0, -0x1.07abe1db13cadp-55 },
  { 0x1.56f4736b527dap+0, 0x1.9bb2c011d93adp-54 },
  { 0x1.5ab07dd485429p+0, 0x1.6324c054647adp-54 },
  { 0x1.5e76f15ad2148p+0, 0x1.ba6f93080e65ep-54 },
  { 0x1.6247eb03a5585p+0, -0x1.383c17e40b497p-54 },
  { 0x1.6623882552225p+0, -0x1.bb60987591c34p-54 },
  { 0x1.6a09e667f3bcdp+0, -0x1.bdd3413b26456p-54 },
  { 0x1.6dfb23c651a2fp+0, -0x1.bbe3a683c88abp-57 },
  { 0x1.71f75e8ec5f74p+0, -0x1.16e4786887a99p-55 },
  { 0x1.75feb564267c9p+0, -0x1.0245957316dd3p-54 },
  { 0x1.7a11473eb0187p+0, -0x1.41577ee04992fp-55 },
  { 0x1.7e2f336cf4e62p+0, 0x1.05d02ba15797ep-56 },
  { 0x1.82589994cce13p+0, -0x1.d4c1dd41532d8p-54 },
  { 0x1.868d99b4492edp+0, -0x1.fc6f89bd4f6bap-54 },
  { 0x1.8ace5422aa0dbp+0, 0x1.6e9f156864b27p-54 },
  { 0x1.8f1ae99157736p+0, 0x1.5cc13a2e3976cp-55 },
  { 0x1.93737b0cdc5e5p+0, -0x1.75fc781b57ebcp-57 },
  { 0x1.97d829fde4e5p+0, -0x1.d185b7c1b85d1p-54 },
  { 0x1.9c49182a3f09p+0, 0x1.c7c46b071f2bep-56 },
  { 0x1.a0c667b5de565p+0, -0x1.359495d1cd533p-54 },
  { 0x1.a5503b23e255dp+0, -0x1.d2f6edb8d41e1p-54 },
  { 0x1.a9e6b5579fdbfp+0, 0x1.0fac90ef7fd31p-54 },
  { 0x1.ae89f995ad3adp+0, 0x1.7a1cd345dcc81p-54 },
  { 0x1.b33a2b84f15fbp+0, -0x1.2805e3084d708p-57 },
  { 0x1.b7f76f2fb5e47p+0, -0x1.5584f7e54ac3bp-56 },
  { 0x1.bcc1e904bc1d2p+0, 0x1.23dd07a2d9e84p-55 },
  { 0x1.c199bdd85529cp+0, 0x1.11065895048ddp-55 },
  { 0x1.c67f12e57d14bp+0, 0x1.2884dff483cadp-54 },
  { 0x1.cb720dcef9069p+0, 0x1.503cbd1e949dbp-56 },
  { 0x1.d072d4a07897cp+0, -0x1.cbc3743797a9cp-54 },
  { 0x1.d5818dcfba487p+0, 0x1.2ed02d75b3707p-55 },
  { 0x1.da9e603db3285p+0, 0x1.c2300696db532p-54 },
  { 0x1.dfc97337b9b5fp+0, -0x1.1a5cd4f184b5cp-54 },
  { 0x1.e502ee78b3ff6p+0, 0x1.39e8980a9cc8fp-55 },
  { 0x1.ea4afa2a490dap+0, -0x1.e9c23179c2893p-54 },
  { 0x1.efa1bee615a27p+0, 0x1.dc7f486a4b6bp-54 },
  { 0x1.f50765b6e454p+0, 0x1.9d3e12dd8a18bp-54 },
  { 0x1.fa7c1819e90d8p+0, 0x1.74853f3a5931ep-55 },
};

/* For 0 <= i < 64, T2[i] = (h,l) such that h+l is the best double-double
   approximation of 2^(i/2^12). The approximation error is bounded as follows:
   |h + l - 2^(i/2^12)| < 2^-107. */
const double __exp2m1_t2[64][2] = {
  { 0x1p+0, 0x0p+0 },
  { 0x1.000b175effdc7p+0, 0x1.ae8e38c59c72ap-54 },
  { 0x1.00162f3904052p+0, -0x1.7b5d0d58ea8f4p-58 },
  { 0x1.0021478e11ce6p+0, 0x1.4115cb6b16a8ep-54 },
  { 0x1.002c605e2e8cfp+0, -0x1.d7c96f201bb2fp-55 },
  { 0x1.003779a95f959p+0, 0x1.84711d4c35e9fp-54 },
  { 0x1.0042936faa3d8p+0, -0x1.0484245243777p-55 },
  { 0x1.004dadb113dap+0, -0x1.4b237da2025f9p-54 },
  { 0x1.0058c86da1c0ap+0, -0x1.5e00e62d6b30dp-56 },
  { 0x1.0063e3a559473p+0, 0x1.a1d6cedbb9481p-54 },
  { 0x1.006eff583fc3dp+0, -0x1.4acf197a00142p-54 },
  { 0x1.007a1b865a8cap+0, -0x1.eaf2ea42391a5p-57 },
  { 0x1.0085382faef83p+0, 0x1.da93f90835f75p-56 },
  { 0x1.00905554425d4p+0, -0x1.6a79084ab093cp-55 },
  { 0x1.009b72f41a12bp+0, 0x1.86364f8fbe8f8p-54 },
  { 0x1.00a6910f3b6fdp+0, -0x1.82e8e14e3110ep-55 },
  { 0x1.00b1afa5abcbfp+0, -0x1.4f6b2a7609f71p-55 },
  { 0x1.00bcceb7707ecp+0, -0x1.e1a258ea8f71bp-56 },
  { 0x1.00c7ee448ee02p+0, 0x1.4362ca5bc26f1p-56 },
  { 0x1.00d30e4d0c483p+0, 0x1.095a56c919d02p-54 },
  { 0x1.00de2ed0ee0f5p+0, -0x1.406ac4e81a645p-57 },
  { 0x1.00e94fd0398ep+0, 0x1.b5a6902767e09p-54 },
  { 0x1.00f4714af41d3p+0, -0x1.91b2060859321p-54 },
  { 0x1.00ff93412315cp+0, 0x1.427068ab22306p-55 },
  { 0x1.010ab5b2cbd11p+0, 0x1.c1d0660524e08p-54 },
  { 0x1.0115d89ff3a8bp+0, -0x1.e7bdfb3204be8p-54 },
  { 0x1.0120fc089ff63p+0, 0x1.843aa8b9cbbc6p-55 },
  { 0x1.012c1fecd613bp+0, -0x1.34104ee7edae9p-56 },
  { 0x1.0137444c9b5b5p+0, -0x1.2b6aeb6176892p-56 },
  { 0x1.01426927f5278p+0, 0x1.a8cd33b8a1bb3p-56 },
  { 0x1.014d8e7ee8d2fp+0, 0x1.2edc08e5da99ap-56 },
  { 0x1.0158b4517bb88p+0, 0x1.57ba2dc7e0c73p-55 },
  { 0x1.0163da9fb3335p+0, 0x1.b61299ab8cdb7p-54 },
  { 0x1.016f0169949edp+0, -0x1.90565902c5f44p-54 },
  { 0x1.017a28af25567p+0, 0x1.70fc41c5c2d53p-55 },
  { 0x1.018550706ab62p+0, 0x1.4b9a6e145d76cp-54 },
  { 0x1.019078ad6a19fp+0, -0x1.008eff5142bf9p-56 },
  { 0x1.019ba16628de2p+0, -0x1.77669f033c7dep-54 },
  { 0x1.01a6ca9aac5f3p+0, -0x1.09bb78eeead0ap-54 },
  { 0x1.01b1f44af9f9ep+0, 0x1.371231477ece5p-54 },
  { 0x1.01bd1e77170b4p+0, 0x1.5e7626621eb5bp-56 },
  { 0x1.01c8491f08f08p+0, -0x1.bc72b100828a5p-54 },
  { 0x1.01d37442d507p+0, -0x1.ce39cbbab8bbep-57 },
  { 0x1.01de9fe280ac8p+0, 0x1.16996709da2e2p-55 },
  { 0x1.01e9cbfe113efp+0, -0x1.c11f5239bf535p-55 },
  { 0x1.01f4f8958c1c6p+0, 0x1.e1d4eb5edc6b3p-55 },
  { 0x1.020025a8f6a35p+0, -0x1.afb99946ee3fp-54 },
  { 0x1.020b533856324p+0, -0x1.8f06d8a148a32p-54 },
  { 0x1.02168143b0281p+0, -0x1.2bf310fc54eb6p-55 },
  { 0x1.0221afcb09e3ep+0, -0x1.c95a035eb4175p-54 },
  { 0x1.022cdece68c4fp+0, -0x1.491793e46834dp-54 },
  { 0x1.02380e4dd22adp+0, -0x1.3e8d0d9c49091p-56 },
  { 0x1.02433e494b755p+0, -0x1.314aa16278aa3p-54 },
  { 0x1.024e6ec0da046p+0, 0x1.48daf888e9651p-55 },
  { 0x1.02599fb483385p+0, 0x1.56dc8046821f4p-55 },
  { 0x1.0264d1244c719p+0, 0x1.45b42356b9d47p-54 },
  { 0x1.027003103b10ep+0, -0x1.082ef51b61d7ep-56 },
  { 0x1.027b357854772p+0, 0x1.2106ed0920a34p-56 },
  { 0x1.0286685c9e059p+0, -0x1.fd4cf26ea5d0fp-54 },
  { 0x1.02919bbd1d1d8p+0, -0x1.09f8775e78084p-54 },
  { 0x1.029ccf99d720ap+0, 0x1.64cbba902ca27p-58 },
  { 0x1.02a803f2d170dp+0, 0x1.4383ef231d207p-54 },
  { 0x1.02b338c811703p+0, 0x1.4a47a505b3a47p-54 },
  { 0x1.02be6e199c811p+0, 0x1.e47120223467fp-54 },
};

/*
  Approximation of exp(x), where x = xh + xl

  exp(x) is approximated by hi + lo.

  For the error analysis, we only consider the case where x^y does not
  overflow or underflow. We get:

  (hi + lo) / exp(xh + xl) = 1 + eps with |eps| < 2^-74.139

  Assumes |xl/xh| < 2^-23.89 and |xl| < 2^-14.3486.

  See analysis before the exp_1() call in cr_pow(), which proves
  |rl| < 2^-23.89 |rh| (here xh=rh and xl=rl).

  At output, we also have 0.99985 < hi+lo < 1.99995 and |lo/hi| < 2^-41.4.
*/

void
__exp2m1_exp_1 (double *hi, double *lo, double xh, double xl)
{
#define INVLOG2 0x1.71547652b82fep+12 /* |INVLOG2-2^12/log(2)| < 2^-43.4 */
  double k = roundeven_finite (xh * INVLOG2);

  double kh, kl;
#define LOG2H 0x1.62e42fefa39efp-13
#define LOG2L 0x1.abc9e3b39803fp-68
  s_mul (&kh, &kl, k, LOG2H, LOG2L);

  double yh, yl;
  fast_two_sum (&yh, &yl, xh - kh, xl);
  yl -= kl;

  int64_t K = k; /* Note: k is an integer, this is just a conversion. */
  int64_t M = (K >> 12) + 0x3ff;
  int64_t i2 = (K >> 6) & 0x3f;
  int64_t i1 = K & 0x3f;

  double t1h = __exp2m1_t1[i2][0], t1l = __exp2m1_t1[i2][1];
  double t2h = __exp2m1_t2[i1][0], t2l = __exp2m1_t2[i1][1];
  d_mul (hi, lo, t2h, t2l, t1h, t1l);

  double qh, ql;
  q_1 (&qh, &ql, yh, yl);

  d_mul (hi, lo, *hi, *lo, qh, ql);

  /* Scale by 2^k. Warning: for x near 1024, we can have k=2^22, thus
     M = 2047, which encodes Inf */
  if (__glibc_unlikely (M == 0x7ff))
    {
      *hi *= 2.0;
      *lo *= 2.0;
      M--;
    }
  double d = asdouble ((uint64_t) M << MANTISSA_WIDTH);
  *hi *= d;
  *lo *= d;
}

static const double Q_2[] = {
  0x1p0,  // degree 0, Q_2[0]
  0x1p0,  // degree 1, Q_2[1]
  0x1p-1, // degree 2, Q_2[2]
  0x1.5555555555555p-3,
  0x1.55555555c4d26p-57, // degree 3, Q_2[3], Q_2[4]
  0x1.5555555555555p-5,	 // degree 4, Q_2[5]
  0x1.1111111111111p-7,	 // degree 5, Q_2[6]
  0x1.6c16c3fbb4213p-10, // degree 6, Q_2[7]
  0x1.a01a023ede0d7p-13, // degree 7, Q_2[8]
};

// Approximation for the accurate path of exp(z) for z=zh+zl,
// with |z| < 0.000130273 < 2^-12.88 and |zl| < 2^-42.6
// (assuming x^y does not overflow or underflow)
void
__exp2m1_q_2 (double *hi, double *lo, double zh, double zl)
{
  /* Let q[0]..q[7] be the coefficients of degree 0..7 of Q_2.
     The ulp of q[7]*z^7 is at most 2^-155, thus we can compute q[7]*z^7
     in double precision only.
     The ulp of q[6]*z^6 is at most 2^-139, thus we can compute q[6]*z^6
     in double precision only.
     The ulp of q[5]*z^5 is at most 2^-124, thus we can compute q[5]*z^5
     in double precision only. */
  double z = zh + zl;
  double q = fma (Q_2[8], zh, Q_2[7]);

  q = fma (q, z, Q_2[6]);

  q = fma (q, z, Q_2[5]);

  // multiply q by z and add Q_2[3] + Q_2[4]
  a_mul (hi, lo, q, z);
  double t;
  fast_two_sum (hi, &t, Q_2[3], *hi);
  *lo += t + Q_2[4];

  // multiply hi+lo by zh+zl and add Q_2[2]
  d_mul (hi, lo, *hi, *lo, zh, zl);
  fast_two_sum (hi, &t, Q_2[2], *hi);
  *lo += t;

  // multiply hi+lo by zh+zl and add Q_2[1]
  d_mul (hi, lo, *hi, *lo, zh, zl);
  fast_two_sum (hi, &t, Q_2[1], *hi);
  *lo += t;

  // multiply hi+lo by zh+zl and add Q_2[0]
  d_mul (hi, lo, *hi, *lo, zh, zl);
  fast_two_sum (hi, &t, Q_2[0], *hi);
  *lo += t;
}

// returns a double-double approximation hi+lo of exp(x*log(2)) for |x| < 745
