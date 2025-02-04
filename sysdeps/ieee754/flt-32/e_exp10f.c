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

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdbit.h>
#include <libm-alias-finite.h>
#include <libm-alias-float.h>
#include <shlib-compat.h>
#include <math-svid-compat.h>
#include "math_config.h"

#define C  __expf2_c
#define TB __expf2_tb

float
__exp10f (float x)
{
  static const double b[] =
    {
      1, 0x1.62e42fef4c4e7p-6, 0x1.ebfd1b232f475p-13, 0x1.c6b19384ecd93p-20
    };
  static const float ex[] =
    {
      10,      100,      1000,      10000,      100000,
      1000000, 10000000, 100000000, 1000000000, 10000000000
    };
  const double iln102 = 0x1.a934f0979a371p+6, iln102h = 0x1.a934f09p+1,
	       iln102l = 0x1.e68dc57f2496p-29;
  uint32_t t = asuint (x);
  double z = x;
  uint32_t ux = t << 1;
  if (__glibc_unlikely (ux > 0x84344134u || ux < 0x72adf1c6u))
    {
      /* ux>0x84344134u: |x| > 0x1.344134p+5 */
      /* ux<0x72adf1c6: |x| < 0x1.adf1c6p-13 */
      if (ux < 0x72adf1c6u)
	return 1.0 + z  * (0x1.26bb1bbb55516p+1 + z
			   * (0x1.53524c73cea69p+1 + z * 0x1.0470591de2ca4p+1));
      if (ux >= 0xffu << 24)
	{ // x is inf or nan
	  if (ux > 0xffu << 24)
	    return x + x; // x = nan
	  static const float ir[] = { INFINITY, 0.0f };
	  return ir[t >> 31]; // x = +-inf
	}
      if (t > 0xc23369f4u)
	{
	  double y = 0x1p-149 + (z + 0x1.66d3e7bd9a403p+5) * 0x1.a934f0979a37p-149;
	  y = fmax (y, 0x1p-151);
	  float r = y;
	  if (r == 0.0f)
	    errno = ERANGE;
	  return r;
	}
      if (t < 0x80000000u)
	{ // x > 0x1.344134p+5
	  float r = 0x1p127f * 0x1p127f;
	  if (r > 0x1.fffffep127f)
	    return __math_oflowf (0);
	  return r;
	}
    }
  if (__glibc_unlikely (!(t << 12)))
    {
      unsigned k = (t >> 20) - 1016;
      if (k <= 26)
	{
	  unsigned int bt = 1 << k, msk = 0x7551101;
	  if (bt & msk)
	    return ex[stdc_count_ones_ui (msk & (bt - 1))];
	}
    }
  double a = iln102 * z, ia = roundeven_finite (a), h = a - ia;
  int64_t ja = ia;
  double sv = asdouble (TB[(ja & 0x1f) * 2] + ((ja >> 5) << 52));
  double h2 = h * h,
	 r = ((b[0] + h * b[1]) + h2 * (b[2] + h * (b[3]))) * (sv);
  float ub = r, lb = r - r * 1.45e-10;
  if (__glibc_unlikely (ub != lb))
    {
      h = (iln102h * z - ia * 0.03125) + iln102l * z;
      double s = sv;
      h2 = h * h;
      double w = s * h;
      r = s + w	* ((C[0] + h * C[1])
		   + h2 * ((C[2] + h * C[3]) + h2 * (C[4] + h * C[5])));
      ub = r;
    }
  return ub;
}
#ifndef __exp10f
strong_alias (__exp10f, __ieee754_exp10f)
libm_alias_finite (__ieee754_exp10f, __exp10f)
/* For architectures that already provided exp10f without SVID support, there
   is no need to add a new version.  */
#if !LIBM_SVID_COMPAT
# define EXP10F_VERSION GLIBC_2_26
#else
# define EXP10F_VERSION GLIBC_2_32
#endif
versioned_symbol (libm, __exp10f, exp10f, EXP10F_VERSION);
libm_alias_float_other (__exp10, exp10)
#endif
