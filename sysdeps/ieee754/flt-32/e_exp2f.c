/* Correctly-rounded 2^x function for binary32 value.

Copyright (c) 2023-2025 Alexei Sibidanov.

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

#include <math.h>
#include <stdint.h>
#include <libm-alias-finite.h>
#include <libm-alias-float.h>
#include <math-use-builtins.h>
#include "math_config.h"

// deal with x=nan, x < -149 and x >= 128
static float
as_special (float x)
{
  uint32_t t = asuint (x);
  uint32_t ux = t << 1;
  if (ux >= 0xffu << 24)
    { // x is inf or nan
      if (ux > 0xffu << 24)
	return x + x; // x = nan
      static const float ir[] = { INFINITY, 0.0f };
      return ir[t >> 31]; // x = +-inf
    }
  if (t >= 0xc3150000u)
    { // x < -149
      double z = x, y = 0x1p-149 + (z + 149) * 0x1p-150;
      y = fmax_finite (y, 0x1p-151);
      // underflow
      return __math_erange (y);
    }
  // now x >= 128
  float r = 0x1p127f * 0x1p127f;
  // overflow
  return __math_erange (r);
}

float
__exp2f (float x)
{
  static const double tb[]
      = { 0x1.0000000000000p+0, 0x1.02c9a3e778061p+0, 0x1.059b0d3158574p+0,
	  0x1.0874518759bc8p+0, 0x1.0b5586cf9890fp+0, 0x1.0e3ec32d3d1a2p+0,
	  0x1.11301d0125b51p+0, 0x1.1429aaea92de0p+0, 0x1.172b83c7d517bp+0,
	  0x1.1a35beb6fcb75p+0, 0x1.1d4873168b9aap+0, 0x1.2063b88628cd6p+0,
	  0x1.2387a6e756238p+0, 0x1.26b4565e27cddp+0, 0x1.29e9df51fdee1p+0,
	  0x1.2d285a6e4030bp+0, 0x1.306fe0a31b715p+0, 0x1.33c08b26416ffp+0,
	  0x1.371a7373aa9cbp+0, 0x1.3a7db34e59ff7p+0, 0x1.3dea64c123422p+0,
	  0x1.4160a21f72e2ap+0, 0x1.44e086061892dp+0, 0x1.486a2b5c13cd0p+0,
	  0x1.4bfdad5362a27p+0, 0x1.4f9b2769d2ca7p+0, 0x1.5342b569d4f82p+0,
	  0x1.56f4736b527dap+0, 0x1.5ab07dd485429p+0, 0x1.5e76f15ad2148p+0,
	  0x1.6247eb03a5585p+0, 0x1.6623882552225p+0, 0x1.6a09e667f3bcdp+0,
	  0x1.6dfb23c651a2fp+0, 0x1.71f75e8ec5f74p+0, 0x1.75feb564267c9p+0,
	  0x1.7a11473eb0187p+0, 0x1.7e2f336cf4e62p+0, 0x1.82589994cce13p+0,
	  0x1.868d99b4492edp+0, 0x1.8ace5422aa0dbp+0, 0x1.8f1ae99157736p+0,
	  0x1.93737b0cdc5e5p+0, 0x1.97d829fde4e50p+0, 0x1.9c49182a3f090p+0,
	  0x1.a0c667b5de565p+0, 0x1.a5503b23e255dp+0, 0x1.a9e6b5579fdbfp+0,
	  0x1.ae89f995ad3adp+0, 0x1.b33a2b84f15fbp+0, 0x1.b7f76f2fb5e47p+0,
	  0x1.bcc1e904bc1d2p+0, 0x1.c199bdd85529cp+0, 0x1.c67f12e57d14bp+0,
	  0x1.cb720dcef9069p+0, 0x1.d072d4a07897cp+0, 0x1.d5818dcfba487p+0,
	  0x1.da9e603db3285p+0, 0x1.dfc97337b9b5fp+0, 0x1.e502ee78b3ff6p+0,
	  0x1.ea4afa2a490dap+0, 0x1.efa1bee615a27p+0, 0x1.f50765b6e4540p+0,
	  0x1.fa7c1819e90d8p+0 };

  uint32_t t = asuint (x);
  if (__glibc_unlikely ((t & 0xffff) == 0))
    {					// x maybe integer
      int k = ((t >> 23) & 0xff) - 127; // 2^k <= |x| < 2^(k+1)
      if (__glibc_unlikely (k >= 0 && k < 9 && (t << (9 + k)) == 0))
	{
	  // x integer, with 1 <= |x| < 2^9
	  int msk = (int) t >> 31;
	  int m = ((t & 0x7fffff) | (1 << 23)) >> (23 - k);
	  m = (m ^ msk) - msk + 127;
	  if (m > 0 && m < 255)
	    {
	      t = m << 23;
	      return asfloat (t);
	    }
	  else if (m <= 0 && m > -23)
	    {
	      /* If f(x) underflows but is exact, no underflow exception should
		 be raised (cf IEEE 754-2019). */
	      t = 1 << (22 + m);
	      return asfloat (t);
	    }
	}
    }
  uint32_t ux = t << 1;
  if (__glibc_unlikely (ux >= 0x86000000u || ux < 0x65000000u))
    {
      // |x| >= 128 or x=nan or |x| < 0x1p-26
      if (__glibc_likely (ux < 0x65000000u))
	return 1.0f + x; // |x| < 0x1p-26
      // if x < -149 or 128 <= x we call as_special()
      if (!(t >= 0xc3000000u && t < 0xc3150000u))
	return as_special (x);
    }
  double offd = 0x1.8p46, xd = x, h = xd - ((xd + offd) - offd), h2 = h * h;
  uint32_t u = asuint (x + 0x1.8p17f);
  uint64_t sv = asuint64 (tb[u & 0x3f]);
  sv += (uint64_t) (u >> 6) << 52;
  static const double b[] = { 1, 0x1.62e42fef4c4e7p-1, 0x1.ebfd1b232f475p-3,
			      0x1.c6b19384ecd93p-5 };
  double svf = asdouble (sv);
  double r = svf * ((b[0] + h * b[1]) + h2 * (b[2] + h * b[3])),
	 eps = 0x1.3d8p-33;
  float ub = r, lb = r - r * eps;
  if (__glibc_likely (ub != lb))
    {
      if (__glibc_unlikely (ux <= 0x79e7526eu))
	{
	  if (t == 0x3b429d37u)
	    return 0x1.00870ap+0f - 0x1p-25f;
	  if (t == 0xbcf3a937u)
	    return 0x1.f58d62p-1f - 0x1p-26f;
	  if (t == 0xb8d3d026u)
	    return 0x1.fff6d2p-1f + 0x1p-26f;
	}
      static const double c[]
	  = { 0x1.62e42fefa39efp-1,  0x1.ebfbdff82c58fp-3,
	      0x1.c6b08d702e0edp-5,  0x1.3b2ab6fb92e5ep-7,
	      0x1.5d886e6d54203p-10, 0x1.430976b8ce6efp-13 };
      r = svf
	  + (svf * h)
		* ((c[0] + h * c[1])
		   + h2 * ((c[2] + h * c[3]) + h2 * (c[4] + h * c[5])));
      ub = r;
    }
  // for x < -126, exp2(x) underflows, whatever the rounding mode
  if (x < -126.0f)
    return __math_erange (ub);
  return ub;
}
#ifndef __exp2f
strong_alias (__exp2f, __ieee754_exp2f)
libm_alias_finite (__ieee754_exp2f, __exp2f)
versioned_symbol (libm, __exp2f, exp2f, GLIBC_2_27);
libm_alias_float_other (__exp2, exp2)
#endif
