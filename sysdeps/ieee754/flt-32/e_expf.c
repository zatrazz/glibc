/* Correctly-rounded natural exponential function for binary32 value.

Copyright (c) 2023-2025 Alexei Sibidanov.

TODO

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

#ifdef __expf
#  undef libm_hidden_proto
#  define libm_hidden_proto(ignored)
#endif

#include <math.h>
#include <stdint.h>
#include <libm-alias-finite.h>
#include <libm-alias-float.h>
#include "math_config.h"
#include "e_expf_data.h"

float
__expf (float x)
{
  const double iln2 = 0x1.71547652b82fep+0, big = 0x1.8p46;
  uint32_t t = asuint (x);
  double z = x, a = iln2 * z;
  uint64_t u = asuint64 (a + big);
  uint32_t ux = t << 1;
  if (__glibc_unlikely (ux > 0x8562e42eu || ux < 0x6f93813eu))
    {
      if (__glibc_likely (ux < 0x6f93813eu))
	return 1.0 + z * (1 + z * 0.5);
      if (ux >= 0xffu << 24)
	{ // x is inf or nan
	  if (ux > 0xffu << 24)
	    return x + x; // x = nan
	  static const float ir[] = { INFINITY, 0.0f };
	  return ir[t >> 31]; // x = +-inf
	}
      if (t > 0xc2ce8ec0u)
	{
	  double y
	      = 0x1p-149 + (z + 0x1.9d1d9fccf477p+6) * 0x1.71547652b82edp-150;
	  y = fmax_finite (y, 0x1p-151);
	  float r = y;
	  if (r == 0.0f)
	    return __math_uflowf (0);
	  return r;
	}
      if (!(t >> 31) && t > 0x42b17217u)
	{
	  float r = 0x1p127f * 0x1p127f;
	  if (r > 0x1.fffffep127f)
	    __math_oflowf (0);
	  return r;
	}
    }
  double ia = big - asdouble (u), h = a + ia;
  double sv = asdouble (asuint64 (TB[u & 0x3f]) + ((u >> 6) << 52));
  double h2 = h * h,
	 r
	 = ((B_EXP[0] + h * B_EXP[1]) + h2 * (B_EXP[2] + h * (B_EXP[3]))) * sv;
  float ub = r, lb = r - r * 1.45e-10;
  if (__glibc_unlikely (ub != lb))
    {
      const double iln2h = 0x1.7154765p+0, iln2l = 0x1.5c17f0bbbe88p-31;
      h = (iln2h * z + ia) + iln2l * z;
      double s = sv;
      h2 = h * h;
      double w = s * h;
      r = s
	  + w
		* ((C_EXP[0] + h * C_EXP[1])
		   + h2
			 * ((C_EXP[2] + h * C_EXP[3])
			    + h2 * (C_EXP[4] + h * C_EXP[5])));
      ub = r;
    }
  return ub;
}

#ifndef __expf
hidden_def (__expf) strong_alias (__expf, __ieee754_expf)
libm_alias_finite (__ieee754_expf, __expf)
versioned_symbol (libm, __expf, expf, GLIBC_2_27);
libm_alias_float_other (__exp, exp)
#endif
