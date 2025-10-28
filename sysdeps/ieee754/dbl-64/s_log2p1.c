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

/* For 362 <= i <= 724, r[i] = _INVERSE[i-362] is a 10-bit approximation of
   1/x[i], where i*2^-9 <= x[i] < (i+1)*2^-9.
   More precisely r[i] is a 10-bit value such that r[i]*y-1 is representable
   exactly on 53 bits for any y, i*2^-9 <= y < (i+1)*2^-9.
   Moreover |r[i]*y-1| <= 0.00212097167968735. */
static const double _INVERSE[363]= {
    0x1.698p+0, 0x1.688p+0, 0x1.678p+0, 0x1.668p+0, 0x1.658p+0, 0x1.648p+0, 0x1.638p+0,
    0x1.63p+0, 0x1.62p+0, 0x1.61p+0, 0x1.6p+0, 0x1.5fp+0, 0x1.5ep+0, 0x1.5dp+0,
    0x1.5cp+0, 0x1.5bp+0, 0x1.5a8p+0, 0x1.598p+0, 0x1.588p+0, 0x1.578p+0, 0x1.568p+0,
    0x1.56p+0, 0x1.55p+0, 0x1.54p+0, 0x1.53p+0, 0x1.52p+0, 0x1.518p+0, 0x1.508p+0,
    0x1.4f8p+0, 0x1.4fp+0, 0x1.4ep+0, 0x1.4dp+0, 0x1.4cp+0, 0x1.4b8p+0, 0x1.4a8p+0,
    0x1.4ap+0, 0x1.49p+0, 0x1.48p+0, 0x1.478p+0, 0x1.468p+0, 0x1.458p+0, 0x1.45p+0,
    0x1.44p+0, 0x1.43p+0, 0x1.428p+0, 0x1.418p+0, 0x1.41p+0, 0x1.4p+0, 0x1.3f8p+0,
    0x1.3e8p+0, 0x1.3ep+0, 0x1.3dp+0, 0x1.3cp+0, 0x1.3b8p+0, 0x1.3a8p+0, 0x1.3ap+0,
    0x1.39p+0, 0x1.388p+0, 0x1.378p+0, 0x1.37p+0, 0x1.36p+0, 0x1.358p+0, 0x1.35p+0,
    0x1.34p+0, 0x1.338p+0, 0x1.328p+0, 0x1.32p+0, 0x1.31p+0, 0x1.308p+0, 0x1.3p+0,
    0x1.2fp+0, 0x1.2e8p+0, 0x1.2d8p+0, 0x1.2dp+0, 0x1.2c8p+0, 0x1.2b8p+0, 0x1.2bp+0,
    0x1.2ap+0, 0x1.298p+0, 0x1.29p+0, 0x1.28p+0, 0x1.278p+0, 0x1.27p+0, 0x1.26p+0,
    0x1.258p+0, 0x1.25p+0, 0x1.24p+0, 0x1.238p+0, 0x1.23p+0, 0x1.228p+0, 0x1.218p+0,
    0x1.21p+0, 0x1.208p+0, 0x1.2p+0, 0x1.1fp+0, 0x1.1e8p+0, 0x1.1ep+0, 0x1.1dp+0,
    0x1.1c8p+0, 0x1.1cp+0, 0x1.1b8p+0, 0x1.1bp+0, 0x1.1ap+0, 0x1.198p+0, 0x1.19p+0,
    0x1.188p+0, 0x1.18p+0, 0x1.17p+0, 0x1.168p+0, 0x1.16p+0, 0x1.158p+0, 0x1.15p+0,
    0x1.14p+0, 0x1.138p+0, 0x1.13p+0, 0x1.128p+0, 0x1.12p+0, 0x1.118p+0, 0x1.11p+0,
    0x1.1p+0, 0x1.0f8p+0, 0x1.0fp+0, 0x1.0e8p+0, 0x1.0ep+0, 0x1.0d8p+0, 0x1.0dp+0,
    0x1.0c8p+0, 0x1.0cp+0, 0x1.0bp+0, 0x1.0a8p+0, 0x1.0ap+0, 0x1.098p+0, 0x1.09p+0,
    0x1.088p+0, 0x1.08p+0, 0x1.078p+0, 0x1.07p+0, 0x1.068p+0, 0x1.06p+0, 0x1.058p+0,
    0x1.05p+0, 0x1.048p+0, 0x1.04p+0, 0x1.038p+0, 0x1.03p+0, 0x1.028p+0, 0x1.02p+0,
    0x1.018p+0, 0x1.01p+0, 0x1.008p+0, 0x1.ff8p-1, 0x1.fe8p-1, 0x1.fd8p-1, 0x1.fc8p-1,
    0x1.fb8p-1, 0x1.fa8p-1, 0x1.f98p-1, 0x1.f88p-1, 0x1.f78p-1, 0x1.f68p-1, 0x1.f58p-1,
    0x1.f5p-1, 0x1.f4p-1, 0x1.f3p-1, 0x1.f2p-1, 0x1.f1p-1, 0x1.fp-1, 0x1.efp-1,
    0x1.eep-1, 0x1.edp-1, 0x1.ec8p-1, 0x1.eb8p-1, 0x1.ea8p-1, 0x1.e98p-1, 0x1.e88p-1,
    0x1.e78p-1, 0x1.e7p-1, 0x1.e6p-1, 0x1.e5p-1, 0x1.e4p-1, 0x1.e3p-1, 0x1.e28p-1,
    0x1.e18p-1, 0x1.e08p-1, 0x1.df8p-1, 0x1.dfp-1, 0x1.dep-1, 0x1.ddp-1, 0x1.dcp-1,
    0x1.db8p-1, 0x1.da8p-1, 0x1.d98p-1, 0x1.d9p-1, 0x1.d8p-1, 0x1.d7p-1, 0x1.d6p-1,
    0x1.d58p-1, 0x1.d48p-1, 0x1.d38p-1, 0x1.d3p-1, 0x1.d2p-1, 0x1.d1p-1, 0x1.d08p-1,
    0x1.cf8p-1, 0x1.ce8p-1, 0x1.cep-1, 0x1.cdp-1, 0x1.cc8p-1, 0x1.cb8p-1, 0x1.ca8p-1,
    0x1.cap-1, 0x1.c9p-1, 0x1.c88p-1, 0x1.c78p-1, 0x1.c68p-1, 0x1.c6p-1, 0x1.c5p-1,
    0x1.c48p-1, 0x1.c38p-1, 0x1.c3p-1, 0x1.c2p-1, 0x1.c18p-1, 0x1.c08p-1, 0x1.bf8p-1,
    0x1.bfp-1, 0x1.bep-1, 0x1.bd8p-1, 0x1.bc8p-1, 0x1.bcp-1, 0x1.bbp-1, 0x1.ba8p-1,
    0x1.b98p-1, 0x1.b9p-1, 0x1.b8p-1, 0x1.b78p-1, 0x1.b68p-1, 0x1.b6p-1, 0x1.b58p-1,
    0x1.b48p-1, 0x1.b4p-1, 0x1.b3p-1, 0x1.b28p-1, 0x1.b18p-1, 0x1.b1p-1, 0x1.bp-1,
    0x1.af8p-1, 0x1.afp-1, 0x1.aep-1, 0x1.ad8p-1, 0x1.ac8p-1, 0x1.acp-1, 0x1.ab8p-1,
    0x1.aa8p-1, 0x1.aap-1, 0x1.a9p-1, 0x1.a88p-1, 0x1.a8p-1, 0x1.a7p-1, 0x1.a68p-1,
    0x1.a6p-1, 0x1.a5p-1, 0x1.a48p-1, 0x1.a4p-1, 0x1.a3p-1, 0x1.a28p-1, 0x1.a2p-1,
    0x1.a1p-1, 0x1.a08p-1, 0x1.ap-1, 0x1.9fp-1, 0x1.9e8p-1, 0x1.9ep-1, 0x1.9dp-1,
    0x1.9c8p-1, 0x1.9cp-1, 0x1.9bp-1, 0x1.9a8p-1, 0x1.9ap-1, 0x1.998p-1, 0x1.988p-1,
    0x1.98p-1, 0x1.978p-1, 0x1.968p-1, 0x1.96p-1, 0x1.958p-1, 0x1.95p-1, 0x1.94p-1,
    0x1.938p-1, 0x1.93p-1, 0x1.928p-1, 0x1.92p-1, 0x1.91p-1, 0x1.908p-1, 0x1.9p-1,
    0x1.8f8p-1, 0x1.8e8p-1, 0x1.8ep-1, 0x1.8d8p-1, 0x1.8dp-1, 0x1.8c8p-1, 0x1.8b8p-1,
    0x1.8bp-1, 0x1.8a8p-1, 0x1.8ap-1, 0x1.898p-1, 0x1.888p-1, 0x1.88p-1, 0x1.878p-1,
    0x1.87p-1, 0x1.868p-1, 0x1.86p-1, 0x1.85p-1, 0x1.848p-1, 0x1.84p-1, 0x1.838p-1,
    0x1.83p-1, 0x1.828p-1, 0x1.82p-1, 0x1.81p-1, 0x1.808p-1, 0x1.8p-1, 0x1.7f8p-1,
    0x1.7fp-1, 0x1.7e8p-1, 0x1.7ep-1, 0x1.7d8p-1, 0x1.7c8p-1, 0x1.7cp-1, 0x1.7b8p-1,
    0x1.7bp-1, 0x1.7a8p-1, 0x1.7ap-1, 0x1.798p-1, 0x1.79p-1, 0x1.788p-1, 0x1.78p-1,
    0x1.778p-1, 0x1.77p-1, 0x1.76p-1, 0x1.758p-1, 0x1.75p-1, 0x1.748p-1, 0x1.74p-1,
    0x1.738p-1, 0x1.73p-1, 0x1.728p-1, 0x1.72p-1, 0x1.718p-1, 0x1.71p-1, 0x1.708p-1,
    0x1.7p-1, 0x1.6f8p-1, 0x1.6fp-1, 0x1.6e8p-1, 0x1.6ep-1, 0x1.6d8p-1, 0x1.6dp-1,
    0x1.6c8p-1, 0x1.6cp-1, 0x1.6b8p-1, 0x1.6bp-1, 0x1.6a8p-1, 0x1.6ap-1,
};

/* For 362 <= i <= 724, (h,l) = _LOG_INV[i-362] is a double-double
   approximation of -log(r) with r=INVERSE[i-362]), with h an integer multiple
   of 2^-42, and |l| < 2^-43. The maximal difference between -log(r) and h+l
   is bounded by 1/2 ulp(l) < 2^-97. */
static const double _LOG_INV[363][2] = {
    {-0x1.615ddb4becp-2, -0x1.3c7ca90bc04b2p-46},
    {-0x1.5e87b20c29p-2, -0x1.527d18f7738fap-44},
    {-0x1.5baf846aa2p-2, 0x1.39ae8f873fa41p-44},
    {-0x1.58d54f86ep-2, -0x1.791f30a795215p-45},
    {-0x1.55f9107a44p-2, 0x1.1e64778df4a62p-46},
    {-0x1.531ac457eep-2, -0x1.df83b7d931501p-44},
    {-0x1.503a682cb2p-2, 0x1.a68c8f16f9b5dp-45},
    {-0x1.4ec97326p-2, -0x1.34d7aaf04d104p-45},
    {-0x1.4be5f95778p-2, 0x1.d7c92cd9ad824p-44},
    {-0x1.4900680401p-2, 0x1.8bccffe1a0f8cp-44},
    {-0x1.4618bc21c6p-2, 0x1.3d82f484c84ccp-46},
    {-0x1.432ef2a04fp-2, 0x1.fb129931715adp-44},
    {-0x1.404308686ap-2, -0x1.f8ef43049f7d3p-44},
    {-0x1.3d54fa5c1fp-2, -0x1.c3e1cd9a395e3p-44},
    {-0x1.3a64c55694p-2, -0x1.7a71cbcd735dp-44},
    {-0x1.3772662bfep-2, 0x1.e9436ac53b023p-44},
    {-0x1.35f865c933p-2, 0x1.b07de4ea1a54ap-44},
    {-0x1.3302c16586p-2, -0x1.6217dc2a3e08bp-44},
    {-0x1.300aead063p-2, -0x1.42f568b75fcacp-44},
    {-0x1.2d10dec508p-2, -0x1.60c61f7088353p-44},
    {-0x1.2a1499f763p-2, 0x1.0dbbf51f3aadcp-44},
    {-0x1.2895a13de8p-2, -0x1.a8d7ad24c13fp-44},
    {-0x1.2596010df7p-2, -0x1.8e7bc224ea3e3p-44},
    {-0x1.22941fbcf8p-2, 0x1.a6976f5eb0963p-44},
    {-0x1.1f8ff9e48ap-2, -0x1.7946c040cbe77p-45},
    {-0x1.1c898c169ap-2, 0x1.81410e5c62affp-44},
    {-0x1.1b05791f08p-2, 0x1.2dd466dc55e2dp-44},
    {-0x1.17fb98e151p-2, 0x1.a8a8ba74a2684p-44},
    {-0x1.14ef67f887p-2, 0x1.e97a65dfc9794p-44},
    {-0x1.136870293bp-2, 0x1.d3e8499d67123p-44},
    {-0x1.1058bf9ae5p-2, 0x1.4ab9d817d52cdp-44},
    {-0x1.0d46b579abp-2, -0x1.d2c81f640e1e6p-44},
    {-0x1.0a324e2739p-2, -0x1.c6bee7ef4030ep-47},
    {-0x1.08a73667c5p-2, -0x1.ebc1d40c5a329p-44},
    {-0x1.058f3c703fp-2, 0x1.0e866bcd236adp-44},
    {-0x1.0402594b4dp-2, -0x1.036b89ef42d7fp-48},
    {-0x1.00e6c45ad5p-2, -0x1.cc68d52e01203p-50},
    {-0x1.fb9186d5e4p-3, 0x1.d572aab993c87p-47},
    {-0x1.f871b28956p-3, 0x1.f75fd6a526efep-44},
    {-0x1.f22e5e72f2p-3, 0x1.f454f1417e41fp-44},
    {-0x1.ebe61f4dd8p-3, 0x1.3d45330fdca4dp-45},
    {-0x1.e8c0252aa6p-3, 0x1.6805b80e8e6ffp-45},
    {-0x1.e27076e2bp-3, 0x1.a342c2af0003cp-44},
    {-0x1.dc1bca0abep-3, -0x1.8fac1a628ccc6p-44},
    {-0x1.d8ef91af32p-3, 0x1.5105fc364c784p-46},
    {-0x1.d293581b6cp-3, 0x1.83270128aaa5fp-44},
    {-0x1.cf6354e09cp-3, -0x1.771239a07d55bp-45},
    {-0x1.c8ff7c79aap-3, 0x1.7794f689f8434p-45},
    {-0x1.c5cba543aep-3, -0x1.0929decb454fcp-45},
    {-0x1.bf601bb0e4p-3, -0x1.386a947c378b5p-45},
    {-0x1.bc286742d8p-3, -0x1.9ac53f39d121cp-44},
    {-0x1.b5b519e8fcp-3, 0x1.4b722ec011f31p-44},
    {-0x1.af3c94e80cp-3, 0x1.a4e633fcd9066p-52},
    {-0x1.abfe5ae462p-3, 0x1.b68f5395f139dp-44},
    {-0x1.a57df28244p-3, -0x1.b99c8ca1d9abbp-44},
    {-0x1.a23bc1fe2cp-3, 0x1.539cd91dc9f0bp-44},
    {-0x1.9bb362e7ep-3, 0x1.1f2a8a1ce0ffcp-45},
    {-0x1.986d322818p-3, -0x1.93b564dd44p-48},
    {-0x1.91dcc8c34p-3, -0x1.7bc6abddeff46p-44},
    {-0x1.8e928de886p-3, -0x1.a8154b13d72d5p-44},
    {-0x1.87fa06520cp-3, -0x1.22120401202fcp-44},
    {-0x1.84abb75866p-3, 0x1.d8daadf4e2bd2p-44},
    {-0x1.815c0a1436p-3, 0x1.02a52f9201ce8p-44},
    {-0x1.7ab890210ep-3, 0x1.bdb9072534a58p-45},
    {-0x1.7764c128f2p-3, -0x1.274903479e3d1p-47},
    {-0x1.70b8f97a1ap-3, -0x1.4ea64f6a95befp-44},
    {-0x1.6d60fe719ep-3, 0x1.bc6e557134767p-44},
    {-0x1.66acd4272ap-3, -0x1.aa1bdbfc6c785p-44},
    {-0x1.6350a28aaap-3, -0x1.d5ec0ab8163afp-45},
    {-0x1.5ff3070a7ap-3, 0x1.8586f183bebf2p-44},
    {-0x1.59338d9982p-3, -0x1.0ba68b7555d4ap-48},
    {-0x1.55d1ad4232p-3, -0x1.add94dda647e8p-44},
    {-0x1.4f099f4a24p-3, 0x1.e9bf2fafeaf27p-44},
    {-0x1.4ba36f39a6p-3, 0x1.4354bb3f219e5p-44},
    {-0x1.483bccce6ep-3, -0x1.eea52723f6369p-46},
    {-0x1.41682bf728p-3, 0x1.10047081f849dp-45},
    {-0x1.3dfc2b0eccp-3, -0x1.8a72a62b8c13fp-45},
    {-0x1.371fc201e8p-3, -0x1.ee8779b2d8abcp-44},
    {-0x1.33af57577p-3, -0x1.c9ecca2fe72a5p-44},
    {-0x1.303d718e48p-3, 0x1.680b5ce3ecb05p-50},
    {-0x1.29552f82p-3, 0x1.5b967f4471dfcp-44},
    {-0x1.25ded0abc6p-3, -0x1.5a3854f176449p-44},
    {-0x1.2266f190a6p-3, 0x1.4d20ab840e7f6p-45},
    {-0x1.1b72ad52f6p-3, -0x1.e80a41811a396p-45},
    {-0x1.17f6458fcap-3, -0x1.843fad093c8dcp-45},
    {-0x1.1478584674p-3, -0x1.563451027c75p-46},
    {-0x1.0d77e7cd08p-3, -0x1.cb2cd2ee2f482p-44},
    {-0x1.09f561ee72p-3, 0x1.8f3057157d1a8p-45},
    {-0x1.0671512ca6p-3, 0x1.a47579cdc0a3dp-45},
    {-0x1.02ebb42bf4p-3, 0x1.5a8fa5ce00e5dp-46},
    {-0x1.f7b79fec38p-4, 0x1.10987e897ed01p-47},
    {-0x1.f0a30c0118p-4, 0x1.d599e83368e91p-44},
    {-0x1.e98b54967p-4, -0x1.4677489c50e97p-44},
    {-0x1.e27076e2bp-4, 0x1.a342c2af0003cp-45},
    {-0x1.d4313d66ccp-4, 0x1.9454379135713p-45},
    {-0x1.cd0cdbf8cp-4, -0x1.3e14db50dd743p-44},
    {-0x1.c5e548f5bcp-4, -0x1.d0c57585fbe06p-46},
    {-0x1.b78c82bb1p-4, 0x1.25ef7bc3987e7p-44},
    {-0x1.b05b49bee4p-4, -0x1.ff22c18f84a5ep-47},
    {-0x1.a926d3a4acp-4, -0x1.563650bd22a9cp-44},
    {-0x1.a1ef1d806p-4, -0x1.cd4176df97bcbp-44},
    {-0x1.9ab4246204p-4, 0x1.8a64826787061p-45},
    {-0x1.8c345d6318p-4, -0x1.b20f5acb42a66p-44},
    {-0x1.84ef898e84p-4, 0x1.7d5cd246977c9p-44},
    {-0x1.7da766d7bp-4, -0x1.2cc844480c89bp-44},
    {-0x1.765bf23a6cp-4, 0x1.ecbc035c4256ap-48},
    {-0x1.6f0d28ae58p-4, 0x1.4b4641b664613p-44},
    {-0x1.60658a9374p-4, -0x1.0c3b1dee9c4f8p-44},
    {-0x1.590cafdfp-4, -0x1.c284f5722abaap-44},
    {-0x1.51b073f06p-4, -0x1.83f69278e686ap-44},
    {-0x1.4a50d3aa1cp-4, 0x1.f7fe1308973e2p-45},
    {-0x1.42edcbea64p-4, -0x1.bc0eeea7c9acdp-46},
    {-0x1.341d7961bcp-4, -0x1.1d0929983761p-44},
    {-0x1.2cb0283f5cp-4, -0x1.e1ee2ca657021p-44},
    {-0x1.253f62f0ap-4, -0x1.416f8fb69a701p-44},
    {-0x1.1dcb263dbp-4, -0x1.9444f5e9e8981p-44},
    {-0x1.16536eea38p-4, 0x1.47c5e768fa309p-46},
    {-0x1.0ed839b554p-4, 0x1.901f46d48abb4p-44},
    {-0x1.075983599p-4, 0x1.b8ecfe4b59987p-44},
    {-0x1.f0a30c0118p-5, 0x1.d599e83368e91p-45},
    {-0x1.e19070c278p-5, 0x1.fea4664629e86p-45},
    {-0x1.d276b8adbp-5, -0x1.6a423c78a64bp-46},
    {-0x1.c355dd092p-5, -0x1.f2ccc9abf8388p-45},
    {-0x1.b42dd71198p-5, 0x1.c827ae5d6704cp-46},
    {-0x1.a4fe9ffa4p-5, 0x1.6e584a0402925p-44},
    {-0x1.95c830ec9p-5, 0x1.c148297c5feb8p-45},
    {-0x1.868a83084p-5, 0x1.2623a134ac693p-46},
    {-0x1.77458f633p-5, 0x1.181dce586af09p-44},
    {-0x1.58a5bafc9p-5, 0x1.b2b739570ad39p-45},
    {-0x1.494acc34d8p-5, -0x1.11c78a56fd247p-45},
    {-0x1.39e87b9fe8p-5, -0x1.eafd480ad9015p-44},
    {-0x1.2a7ec2215p-5, 0x1.78ce77a9163fep-45},
    {-0x1.1b0d98924p-5, 0x1.3401e9ae889bbp-44},
    {-0x1.0b94f7c198p-5, 0x1.e89896f022783p-45},
    {-0x1.f829b0e78p-6, -0x1.980267c7e09e4p-45},
    {-0x1.d91a66c54p-6, -0x1.e61f1658cfb9ap-45},
    {-0x1.b9fc027bp-6, 0x1.b9a010ae6922ap-44},
    {-0x1.9ace7551dp-6, 0x1.d75d97ec7c41p-45},
    {-0x1.7b91b07d6p-6, 0x1.3b955b602ace4p-44},
    {-0x1.5c45a51b9p-6, 0x1.63bb6216d87d8p-45},
    {-0x1.3cea44347p-6, 0x1.6a2c432d6a40bp-44},
    {-0x1.1d7f7eb9fp-6, 0x1.4193a83fcc7a6p-46},
    {-0x1.fc0a8b0fcp-7, -0x1.f1e7cf6d3a69cp-50},
    {-0x1.bcf712c74p-7, -0x1.c25e097bd9771p-46},
    {-0x1.7dc475f82p-7, 0x1.eb1245b5da1f5p-44},
    {-0x1.3e7295d26p-7, 0x1.609c1ff29a114p-45},
    {-0x1.fe02a6b1p-8, -0x1.9e23f0dda40e4p-46},
    {-0x1.7ee11ebd8p-8, -0x1.749d3c2d23a07p-47},
    {-0x1.ff00aa2bp-9, -0x1.0bc04a086b56ap-45},
    {-0x1.ff802a9bp-10, 0x1.3bc661d61c5ebp-44},
    {0x1.00200556p-10, 0x1.56224cd5f35f8p-44},
    {0x1.809048288p-9, 0x1.85c0696a70c0cp-45},
    {0x1.40c8a7478p-8, 0x1.e3871df070002p-46},
    {0x1.c189cbb1p-8, -0x1.d80551258856p-44},
    {0x1.2145e939ep-7, 0x1.e3d1238c4eap-44},
    {0x1.61e77e8b6p-7, -0x1.8073eeaf8eaf3p-44},
    {0x1.a2a9c6c18p-7, -0x1.f73bc4d6d3472p-44},
    {0x1.e38ce3034p-7, -0x1.9de88a3da281ap-44},
    {0x1.12487a55p-6, 0x1.fdbe5fed4b393p-44},
    {0x1.32db0ea13p-6, 0x1.710cb130895fcp-45},
    {0x1.537e3f45fp-6, 0x1.ab259d2d7f253p-45},
    {0x1.63d617869p-6, 0x1.7abf389596542p-47},
    {0x1.8492528c9p-6, -0x1.aa0ba325a0c34p-45},
    {0x1.a55f548c6p-6, -0x1.de0709f2d03c9p-45},
    {0x1.c63d2ec15p-6, -0x1.5439ce030a687p-44},
    {0x1.e72bf2814p-6, -0x1.8d75149774d47p-45},
    {0x1.0415d89e78p-5, -0x1.dddc7f461c516p-44},
    {0x1.149e3e4008p-5, -0x1.2b98a9a4168fdp-44},
    {0x1.252f32f8dp-5, 0x1.83e9ae021b67bp-45},
    {0x1.35c8bfaa1p-5, 0x1.8357d5ef9eb35p-44},
    {0x1.3e18c1ca08p-5, 0x1.748ed3f6e378ep-44},
    {0x1.4ebf4334ap-5, -0x1.d9150f73be773p-45},
    {0x1.5f6e73079p-5, -0x1.0485a8012494cp-45},
    {0x1.70265a551p-5, -0x1.888df11fd5ce7p-45},
    {0x1.80e7023d9p-5, -0x1.99dc16f28bf45p-44},
    {0x1.91b073efd8p-5, -0x1.9d7c53f76ca96p-46},
    {0x1.9a187b574p-5, -0x1.0c22e4ec4d90dp-44},
    {0x1.aaef2d0fbp-5, 0x1.0fc1a353bb42ep-45},
    {0x1.bbcebfc69p-5, -0x1.7bf868c317c2ap-46},
    {0x1.ccb73cddd8p-5, 0x1.965c36e09f5fep-44},
    {0x1.dda8adc68p-5, -0x1.1b1ac64d9e42fp-45},
    {0x1.e624c4a0b8p-5, -0x1.0f25c74676689p-44},
    {0x1.f723b518p-5, -0x1.d6eb0dd5610d3p-44},
    {0x1.0415d89e74p-4, 0x1.111c05cf1d753p-46},
    {0x1.0c9e615ac4p-4, 0x1.c2da80974d976p-45},
    {0x1.10e45b3cbp-4, -0x1.7cf69284a3465p-44},
    {0x1.1973bd1464p-4, 0x1.566d154f930b3p-44},
    {0x1.2207b5c784p-4, 0x1.49d8cfc10c7bfp-44},
    {0x1.2aa04a447p-4, 0x1.7a48ba8b1cb41p-44},
    {0x1.2eee507b4p-4, 0x1.8081edd77c86p-47},
    {0x1.378dd7f748p-4, 0x1.7141128f1facap-44},
    {0x1.403207b414p-4, 0x1.6fd84aa8157cp-45},
    {0x1.4485e03dbcp-4, 0x1.fad46e8d26ab7p-44},
    {0x1.4d3115d208p-4, -0x1.53a2582f4e1efp-48},
    {0x1.55e10050ep-4, 0x1.c1d740c53c72ep-47},
    {0x1.5e95a4d978p-4, 0x1.1cb7ce1d17171p-44},
    {0x1.62f1be7d78p-4, -0x1.179957ed63c4ep-45},
    {0x1.6bad83c188p-4, 0x1.daf3cc08926aep-47},
    {0x1.746e100228p-4, -0x1.126d16e1e21d2p-44},
    {0x1.78d02263d8p-4, 0x1.69b5794b69fb7p-47},
    {0x1.8197e2f41p-4, -0x1.c0fe460d20041p-44},
    {0x1.8a6477a91cp-4, 0x1.c28c0af9bd6dfp-44},
    {0x1.8ecc933aecp-4, -0x1.22f39be67f7aap-45},
    {0x1.97a07024ccp-4, -0x1.8bcc1732093cep-48},
    {0x1.a0792e9278p-4, -0x1.a9ce6c9ad51bfp-47},
    {0x1.a4e7640b1cp-4, -0x1.e42b6b94407c8p-47},
    {0x1.adc77ee5bp-4, -0x1.573b209c31904p-44},
    {0x1.b23965a53p-4, -0x1.ff64eea137079p-49},
    {0x1.bb20e936d8p-4, -0x1.68ba835459b8ep-44},
    {0x1.c40d6425a4p-4, 0x1.cb1121d1930ddp-44},
    {0x1.c885801bc4p-4, 0x1.646d1c65aacd3p-45},
    {0x1.d179788218p-4, 0x1.36433b5efbeedp-44},
    {0x1.d5f556592p-4, 0x1.0e239cc185469p-44},
    {0x1.def0d8d468p-4, -0x1.24750412e9a74p-44},
    {0x1.e7f1691a34p-4, -0x1.2c1c59bc77bfap-44},
    {0x1.ec739830ap-4, 0x1.11fcba80cdd1p-44},
    {0x1.f57bc7d9p-4, 0x1.76a6c9ea8b04ep-46},
    {0x1.fa01c9db58p-4, -0x1.8f351fa48a73p-47},
    {0x1.0188d2ecf6p-3, 0x1.3f9651cff9dfep-47},
    {0x1.03cdc0a51ep-3, 0x1.81a9cf169fc5cp-44},
    {0x1.08598b59e4p-3, -0x1.7e5dd7009902cp-45},
    {0x1.0aa0691268p-3, -0x1.45519d7032129p-44},
    {0x1.0f301717dp-3, -0x1.e09b441ae86c5p-44},
    {0x1.13c2605c3ap-3, -0x1.cf5fdd94f6509p-45},
    {0x1.160c8024b2p-3, 0x1.ec2d2a9009e3dp-45},
    {0x1.1aa2b7e24p-3, -0x1.1ac38dde3b366p-44},
    {0x1.1ceed09854p-3, -0x1.15c1c39192af9p-44},
    {0x1.2188fd9808p-3, -0x1.b3a1e7f50c701p-44},
    {0x1.23d712a49cp-3, 0x1.00d238fd3df5cp-46},
    {0x1.28753bc11ap-3, 0x1.7494e359302e6p-44},
    {0x1.2ac55095f6p-3, -0x1.d3466d0c6c8a8p-46},
    {0x1.2f677cbbcp-3, 0x1.52b302160f40dp-44},
    {0x1.31b994d3a4p-3, 0x1.f098ee3a5081p-44},
    {0x1.365fcb015ap-3, -0x1.fd3a0afb9691bp-44},
    {0x1.38b3e9e028p-3, -0x1.70ef0545c17f9p-44},
    {0x1.3d5e3126bcp-3, 0x1.3fb2f85096c4bp-46},
    {0x1.3fb45a5992p-3, 0x1.19713c0cae559p-44},
    {0x1.420b32741p-3, -0x1.16282c85a0884p-46},
    {0x1.46baf0f9f6p-3, -0x1.249cd0790841ap-46},
    {0x1.4913d8333cp-3, -0x1.53e43558124c4p-44},
    {0x1.4dc7b897bcp-3, 0x1.c79b60ae1ff0fp-47},
    {0x1.5022b292f6p-3, 0x1.48a05ff36a25bp-44},
    {0x1.54dabc261p-3, 0x1.746fee5c8d0d8p-45},
    {0x1.5737cc9018p-3, 0x1.9baa7a6b887f6p-44},
    {0x1.5bf406b544p-3, -0x1.27023eb68981cp-46},
    {0x1.5e533144c2p-3, -0x1.1ce0bf3b290eap-44},
    {0x1.60b3100b0ap-3, -0x1.71456c988f814p-44},
    {0x1.6574ebe8c2p-3, -0x1.98c1d34f0f462p-44},
    {0x1.67d6e9d786p-3, -0x1.11e8830a706d3p-44},
    {0x1.6c9d07d204p-3, -0x1.c73fafd9b2dcap-50},
    {0x1.6f0128b756p-3, 0x1.577390d31ef0fp-44},
    {0x1.716600c914p-3, 0x1.51b157cec3838p-49},
    {0x1.7631d82936p-3, -0x1.5e77dc7c5f3e1p-45},
    {0x1.7898d85444p-3, 0x1.8e67be3dbaf3fp-44},
    {0x1.7d6903caf6p-3, -0x1.4c06b17c301d7p-45},
    {0x1.7fd22ff59ap-3, -0x1.58bebf457b7d2p-46},
    {0x1.823c16551ap-3, 0x1.e0ddb9a631e83p-46},
    {0x1.871213750ep-3, 0x1.328eb42f9af75p-44},
    {0x1.897e2b17b2p-3, -0x1.96b37380cbe9ep-45},
    {0x1.8beafeb39p-3, -0x1.73d54aae92cd1p-47},
    {0x1.90c6db9fccp-3, -0x1.935f57718d7cap-46},
    {0x1.9335e5d594p-3, 0x1.3115c3abd47dap-44},
    {0x1.95a5adcf7p-3, 0x1.7f22858a0ff6fp-47},
    {0x1.9a8778debap-3, 0x1.470fa3efec39p-44},
    {0x1.9cf97cdcep-3, 0x1.d862f10c414e3p-44},
    {0x1.9f6c40708ap-3, -0x1.337d94bcd3f43p-44},
    {0x1.a454082e6ap-3, 0x1.60a77c81f7171p-44},
    {0x1.a6c90d44b8p-3, -0x1.f63b7f037b0c6p-44},
    {0x1.a93ed3c8aep-3, -0x1.8724350562169p-45},
    {0x1.ae2ca6f672p-3, 0x1.7a8d5ae54f55p-44},
    {0x1.b0a4b48fc2p-3, -0x1.2e72d5c3998edp-45},
    {0x1.b31d8575bcp-3, 0x1.c794e562a63cbp-44},
    {0x1.b811730b82p-3, 0x1.e90683b9cd768p-46},
    {0x1.ba8c90ae4ap-3, 0x1.a32e7f44432dap-44},
    {0x1.bd087383bep-3, -0x1.d4bc4595412b6p-45},
    {0x1.c2028ab18p-3, -0x1.92e0ee55c7ac6p-45},
    {0x1.c480c0005cp-3, 0x1.9a294d5e44e76p-44},
    {0x1.c6ffbc6fp-3, 0x1.ee138d3a69d43p-44},
    {0x1.c97f8079d4p-3, 0x1.3b161a8c6e6c5p-45},
    {0x1.ce816157f2p-3, -0x1.9e0aba2099515p-45},
    {0x1.d1037f2656p-3, -0x1.84a7e75b6f6e4p-47},
    {0x1.d38666872p-3, -0x1.73650b38932bcp-44},
    {0x1.d88e93fb3p-3, -0x1.75f280234bf51p-44},
    {0x1.db13db0d48p-3, 0x1.2806a847527e6p-44},
    {0x1.dd99edaf6ep-3, -0x1.02ec669c756ebp-44},
    {0x1.e020cc6236p-3, -0x1.52b00adb91424p-45},
    {0x1.e530effe72p-3, -0x1.fdbdbb13f7c18p-44},
    {0x1.e7ba35eb78p-3, -0x1.d5eee23793649p-47},
    {0x1.ea4449f04ap-3, 0x1.5e91663732a36p-44},
    {0x1.eccf2c8feap-3, -0x1.bec63a3e7564p-44},
    {0x1.ef5ade4ddp-3, -0x1.a211565bb8e11p-51},
    {0x1.f474b134ep-3, -0x1.bae49f1df7b5ep-44},
    {0x1.f702d36778p-3, -0x1.0819516673e23p-46},
    {0x1.f991c6cb3cp-3, -0x1.90d04cd7cc834p-44},
    {0x1.fc218be62p-3, 0x1.4bba46f1cf6ap-44},
    {0x1.00a1c6addap-2, 0x1.1cd8d688b9e18p-44},
    {0x1.01eae5626cp-2, 0x1.a43dcfade85aep-44},
    {0x1.03346e0106p-2, 0x1.89ff8a966395cp-48},
    {0x1.047e60cde8p-2, 0x1.dbdf10d397f3cp-45},
    {0x1.05c8be0d96p-2, 0x1.ad0f1c77ccb58p-45},
    {0x1.085eb8f8aep-2, 0x1.e5d513f45fe7bp-44},
    {0x1.09aa572e6cp-2, 0x1.b50a1e1734342p-44},
    {0x1.0af660eb9ep-2, 0x1.3c7c3f528d80ap-45},
    {0x1.0c42d67616p-2, 0x1.7188b163ceae9p-45},
    {0x1.0d8fb813ebp-2, 0x1.ee8c88753fa35p-46},
    {0x1.102ac0a35dp-2, -0x1.f1fbddfdfd686p-45},
    {0x1.1178e8227ep-2, 0x1.1ef78ce2d07f2p-44},
    {0x1.12c77cd007p-2, 0x1.3b2948a11f797p-46},
    {0x1.14167ef367p-2, 0x1.e0c07824daaf5p-44},
    {0x1.1565eed456p-2, -0x1.e75adfb6aba25p-49},
    {0x1.16b5ccbadp-2, -0x1.23299042d74bfp-44},
    {0x1.1956d3b9bcp-2, 0x1.7d2f73ad1aa14p-45},
    {0x1.1aa7fd638dp-2, 0x1.9f60a9616f7ap-45},
    {0x1.1bf99635a7p-2, -0x1.1ac89575c2125p-44},
    {0x1.1d4b9e796cp-2, 0x1.22a667c42e56dp-45},
    {0x1.1e9e16788ap-2, -0x1.82eaed3c8b65ep-44},
    {0x1.1ff0fe7cf4p-2, 0x1.e9d5b513ff0c1p-44},
    {0x1.214456d0ecp-2, -0x1.caf0428b728a3p-44},
    {0x1.23ec5991ecp-2, -0x1.6dbe448a2e522p-44},
    {0x1.25410494e5p-2, 0x1.b1d7ac0ef77f2p-44},
    {0x1.269621134ep-2, -0x1.1b61f10522625p-44},
    {0x1.27ebaf58d9p-2, -0x1.b198800b4bda7p-45},
    {0x1.2941afb187p-2, -0x1.210c2b730e28bp-44},
    {0x1.2a982269a4p-2, -0x1.2058e557285cfp-45},
    {0x1.2bef07cdc9p-2, 0x1.a9cfa4a5004f4p-45},
    {0x1.2d46602addp-2, -0x1.88d0ddcd54196p-45},
    {0x1.2ff66b04ebp-2, -0x1.8aed2541e6e2ep-44},
    {0x1.314f1e1d36p-2, -0x1.8e27ad3213cb8p-45},
    {0x1.32a8456512p-2, 0x1.4f928139af5d6p-47},
    {0x1.3401e12aedp-2, -0x1.17c73556e291dp-44},
    {0x1.355bf1bd83p-2, -0x1.ba99b8964f0e8p-45},
    {0x1.36b6776be1p-2, 0x1.16ecdb0f177c8p-46},
    {0x1.3811728565p-2, -0x1.a71e493a0702bp-45},
    {0x1.396ce359bcp-2, -0x1.5839c5663663dp-47},
    {0x1.3ac8ca38e6p-2, -0x1.d0befbc02be4ap-45},
    {0x1.3c25277333p-2, 0x1.83b54b606bd5cp-46},
    {0x1.3d81fb5947p-2, -0x1.22c7c2a9d37a4p-45},
    {0x1.3edf463c17p-2, -0x1.f067c297f2c3fp-44},
    {0x1.419b423d5fp-2, -0x1.ce379226de3ecp-44},
    {0x1.42f9f3ff62p-2, 0x1.906440f7d3354p-44},
    {0x1.44591e053ap-2, -0x1.6e95892923d88p-47},
    {0x1.45b8c0a17ep-2, -0x1.d9120e7d0a853p-47},
    {0x1.4718dc271cp-2, 0x1.06c18fb4c14c5p-44},
    {0x1.487970e958p-2, 0x1.dc1b8465cf25fp-44},
    {0x1.49da7f3bccp-2, 0x1.07b334daf4b9ap-44},
    {0x1.4b3c077268p-2, -0x1.65b4681052b9fp-46},
    {0x1.4c9e09e173p-2, -0x1.e20891b0ad8a4p-45},
    {0x1.4e0086dd8cp-2, -0x1.4d692a1e44788p-44},
    {0x1.4f637ebbaap-2, -0x1.fc158cb3124b9p-44},
    {0x1.50c6f1d11cp-2, -0x1.a0e6b7e827c2cp-44},
    {0x1.522ae0738ap-2, 0x1.ebe708164c759p-45},
    {0x1.538f4af8f7p-2, 0x1.7ec02e45547cep-45},
    {0x1.54f431b7bep-2, 0x1.a8954c0910952p-46},
    {0x1.5659950695p-2, 0x1.4c5fd2badc774p-46},
    {0x1.57bf753c8dp-2, 0x1.fadedee5d40efp-46},
    {0x1.5925d2b113p-2, -0x1.69bf5a7a56f34p-44},
    {0x1.5a8cadbbeep-2, -0x1.7c79b0af7ecf8p-48},
    {0x1.5bf406b544p-2, -0x1.27023eb68981cp-45},
    {0x1.5d5bddf596p-2, -0x1.a0b2a08a465dcp-47},
    {0x1.5ec433d5c3p-2, 0x1.6b71a1229d17fp-44},
    {0x1.602d08af09p-2, 0x1.ebe9176df3f65p-46},
    {0x1.61965cdb03p-2, -0x1.f08ad603c488ep-45},
    {0x1.630030b3abp-2, -0x1.db623e731aep-45},
};

/* The following is a degree-7 polynomial generated by Sollya
   approximating log(1+x) for |x| < 0.00212097167968735,
   with absolute error < 2^-79.592 and relative error < 2^-70.467
   (see file Prel.sollya).
   The polynomial is P[0]*x + P[1]*x^2 + ... + P[6]*x^7.
   The algorithm assumes that the degree-1 coefficient P[0] is 1,
   and the degree-2 coefficient P[1] is -0.5. */
static const double P[7] = {0x1p0,                 /* degree 1 */
                            -0x1p-1,               /* degree 2 */
                            0x1.555555555555p-2,   /* degree 3 */
                            -0x1.fffffffff572dp-3, /* degree 4 */
                            0x1.999999a2d7868p-3,  /* degree 5 */
                            -0x1.5555c0d31b08ep-3, /* degree 6 */
                            0x1.2476b9058e396p-3,  /* degree 7 */
};

/* The following is a degree-11 polynomial generated by Sollya
   approximating log(1+x) for |x| < 0.03125,
   with absolute error < 2^-73.441 and relative error < 2^-67.088
   (see file Pabs_a.sollya).
   The polynomial is P[0]*x + P[1]*x^2 + ... + P[10]*x^11.
   The algorithm assumes that the degree-1 coefficient P[0] is 1
   and the degree-2 coefficient P[1] is -0.5. */
static const double Pa[11] = {0x1p0,                /* degree 1 */
                             -0x1p-1,               /* degree 2 */
                             0x1.5555555555555p-2,  /* degree 3 */
                             -0x1.ffffffffffe5fp-3, /* degree 4 */
                             0x1.999999999aa82p-3,  /* degree 5 */
                             -0x1.555555583a8c8p-3, /* degree 6 */
                             0x1.2492491c359e6p-3,  /* degree 7 */
                             -0x1.ffffc728edeeap-4, /* degree 8 */
                             0x1.c71c961f3498p-4,   /* degree 9 */
                             -0x1.9a82ac77c05f4p-4, /* degree 10 */
                             0x1.74b40dd1707d3p-4,  /* degree 11 */
};

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
  double p56 = __builtin_fma (P[6], z, P[5]);
  double p34 = __builtin_fma (P[4], z, P[3]);
  double ph = __builtin_fma (p56, z2h, p34);
  /* ph approximates P[3]+P[4]*z+P[5]*z^2+P[6]*z^3 */
  ph = __builtin_fma (ph, z, P[2]);
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
