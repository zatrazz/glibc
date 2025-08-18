/* Correctly-rounded sine of binary32 value.

Copyright (c) 2022-2025 Alexei Sibidanov.

This file is part of the CORE-MATH project
(https://core-math.gitlabpages.inria.fr/).

Permission is hereby granted, free of charge, to any person obtaining A copy
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
#include <math.h>
#include <math-barriers.h>
#include <libm-alias-float.h>
#include "s_sincosf.h"

static float __attribute__ ((noinline))
as_sinf_database (float x, double r)
{
  static const struct
  {
    union
    {
      float arg;
      uint32_t uarg;
    };
    float rh, rl;
  } st[] = {
    { { 0x1.33333p+13 }, -0x1.63f4bap-2, -0x1p-27 },
    { { 0x1.75b8a2p-1 }, 0x1.55688ap-1, -0x1p-26 },
    { { 0x1.4f0654p+0 }, 0x1.ee836cp-1, -0x1p-26 },
    { { 0x1.2d97c8p+3 }, -0x1.99bc5ap-26, -0x1p-51 },
  };
  uint32_t ax = asuint (x) & (~0u >> 1);
  for (unsigned i = 0; i < sizeof (st) / sizeof (st[0]); i++)
    if (__glibc_unlikely (st[i].uarg == ax))
      return add_sign (x, st[i].rh, st[i].rl);
  return r;
}

static float __attribute__ ((noinline))
as_sinf_big (float x)
{
  uint32_t t = asuint (x);
  uint32_t ax = t << 1;
  if (__glibc_unlikely (ax >= 0xffu << 24))
    { // nan or +-inf
      if (ax << 8)
	return x + x; // nan
      return __math_invalidf (x);
    }
  int ia;
  double z = rbig (t, &ia);
  double z2 = z * z, z4 = z2 * z2;
  double aa = (A[0] + z2 * A[1]) + z4 * (A[2] + z2 * A[3]);
  double bb = (B[0] + z2 * B[1]) + z4 * (B[2] + z2 * B[3]);
  double s0 = TB[ia & 31], c0 = TB[(ia + 8u) & 31];
  double r = s0 + z * (aa * c0 - bb * (z * s0));
  return r;
}

#ifndef SECTION
#  define SECTION
#endif

#ifndef SINF
#  define SINF_FUNC __sinf
#else
#  define SINF_FUNC SINF
#endif

float SECTION
SINF_FUNC (float x)
{
  uint32_t ax = asuint (x) << 1;
  int ia;
  double z0 = x, z;
  if (__glibc_unlikely (ax > 0x99000000u || ax < 0x73000000u))
    {
      // |x| > 0x1p+26 or |x| < 0x1p-12
      if (__glibc_likely (ax < 0x73000000u))
	{ // |x| < 0x1p-12
	  if (__glibc_unlikely (ax < 0x66000000u))
	    { // |x| < 0x1p-25
	      if (__glibc_unlikely (ax == 0u))
		return x;
	      float res = fmaf (-x, fabsf (x), x);
	      /* The Taylor expansion of sin(x) at x=0 is x - x^3/6 + o(x^3).
		 For |x| > 2^-126 we have no underflow, whatever the rounding
		 mode. For |x| < 2^-126, since |sin(x)| < |x|, we always have
		 underflow. For |x| = 2^-126, we have underflow for rounding
		 towards zero, i.e., when sin(x) rounds to nextbelow(2^-126).
		 In summary, we have underflow whenever |x|<2^-126 or
		 |res|<2^-126. */
	      if (fabsf (x) < 0x1p-126f || fabsf (res) < 0x1p-126f)
		return __math_erange (res); // underflow
	      return res;
	    }
	  return (-0x1.555556p-3f * x) * (x * x) + x;
	}
      return as_sinf_big (x);
    }
  if (__glibc_likely (ax < 0x822d97c8u))
    {
      if (__glibc_unlikely (ax == 0x7e75b8a2u || ax == 0x7f4f0654u))
	return as_sinf_database (x, 0.0);
      z = rltl0 (z0, &ia);
    }
  else
    {
      if (__glibc_unlikely (ax == 0x8c333330u))
	return as_sinf_database (x, 0.0);
      z = rltl (z0, &ia);
    }
  double z2 = z * z, z4 = z2 * z2;
  double aa = (A[0] + z2 * A[1]) + z4 * (A[2] + z2 * A[3]);
  double bb = (B[0] + z2 * B[1]) + z4 * (B[2] + z2 * B[3]);
  double s0 = TB[ia & 31], c0 = TB[(ia + 8) & 31];
  double r = s0 + aa * (z * c0) - bb * (z2 * s0);
  return r;
}

#ifndef SINF
libm_alias_float (__sin, sin)
#endif
