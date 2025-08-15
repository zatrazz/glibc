/* Correctly-rounded 10^x function for binary32 value.

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
#include <shlib-compat.h>
#include <stdbit.h>
#include <math-svid-compat.h>
#include "math_config.h"
#include "e_exp10f_data.h"

float
__exp10f (float x)
{
  const double iln102 = 0x1.a934f0979a371p+6, iln102h = 0x1.a934f09p+1,
	       iln102l = 0x1.e68dc57f2496p-29;
  uint32_t t = asuint (x);
  double z = x;
  uint32_t ux = t << 1;
  if (__glibc_unlikely (ux > 0x84344134u || ux < 0x72adf1c6u))
    {
      // |x| > 0x1.344134p+5 or x=nan or |x| < 0x1.adf1c6p-13
      if (ux < 0x72adf1c6u) // |x| < 0x1.adf1c6p-13
	return 1.0
	       + z
		     * (0x1.26bb1bbb55516p+1
			+ z
			      * (0x1.53524c73cea69p+1
				 + z * 0x1.0470591de2ca4p+1));
      if (ux >= 0xffu << 24)
	{ // x is inf or nan
	  if (ux > 0xffu << 24)
	    return x + x; // x = nan
	  static const float ir[] = { INFINITY, 0.0f };
	  return ir[t >> 31]; // x = +-inf
	}
      if (t > 0xc23369f4u)
	{ // x < -0x1.66d3e8p+5
	  double y
	      = 0x1p-149 + (z + 0x1.66d3e7bd9a403p+5) * 0x1.a934f0979a37p-149;
	  y = fmax_finite (y, 0x1p-151);
	  /* underflow */
	  return __math_range (y);
	}
      if (t < 0x80000000u)
	{ // x > 0x1.344134p+5
	  float r = 0x1p127f * 0x1p127f;
	  return __math_range (r);
	}
    }
  if (__glibc_unlikely (!(t << 12)))
    {
      unsigned k = (t >> 20) - 1016;
      if (k <= 26)
	{
	  unsigned bt = 1 << k, msk = 0x7551101;
	  if (bt & msk)
	    return EX[stdc_count_ones (msk & (bt - 1))];
	}
    }
  double a = iln102 * z, ia = roundeven_finite (a), h = a - ia;
  int64_t ja = ia;
  double sv = asdouble (TB[ja & 0x1f] + ((uint64_t) (ja >> 5) << 52));
  double h2 = h * h, r = ((B[0] + h * B[1]) + h2 * (B[2] + h * (B[3]))) * (sv);
  float ub = r, lb = r - r * 1.45e-10;
  if (__glibc_unlikely (ub != lb))
    {
      h = (iln102h * z - ia * 0.03125) + iln102l * z;
      double s = sv;
      h2 = h * h;
      double w = s * h;
      r = s
	  + w
		* ((C[0] + h * C[1])
		   + h2 * ((C[2] + h * C[3]) + h2 * (C[4] + h * C[5])));
      ub = r;
    }
#ifdef CORE_MATH_SUPPORT_ERRNO
  // for x <= -0x1.2f7032p+5, exp10(x) underflows, whatever the rounding mode
  if (x <= -0x1.2f7032p+5f)
    errno = ERANGE; // underflow
#endif
  return ub;
}
#ifndef __exp10f
strong_alias (__exp10f, __ieee754_exp10f)
libm_alias_finite (__ieee754_exp10f, __exp10f)
/* For architectures that already provided exp10f without SVID support, there
   is no need to add a new version.  */
#  if !LIBM_SVID_COMPAT
#    define EXP10F_VERSION GLIBC_2_26
#  else
#    define EXP10F_VERSION GLIBC_2_32
#  endif
versioned_symbol (libm, __exp10f, exp10f, EXP10F_VERSION);
libm_alias_float_other (__exp10, exp10)
#endif
