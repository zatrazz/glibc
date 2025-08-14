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
#include "e_expf_data.h"

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
  uint64_t sv = asuint64 (TB[u & 0x3f]);
  sv += (uint64_t) (u >> 6) << 52;
  double svf = asdouble (sv);
  double r
      = svf * ((B_EXP2[0] + h * B_EXP2[1]) + h2 * (B_EXP2[2] + h * B_EXP2[3])),
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
      r = svf
	  + (svf * h)
		* ((C_EXP2[0] + h * C_EXP2[1])
		   + h2
			 * ((C_EXP2[2] + h * C_EXP2[3])
			    + h2 * (C_EXP2[4] + h * C_EXP2[5])));
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
