/* Correctly-rounded cosine of binary32 value.

Copyright (c) 2022-2026 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary32/cosh/coshf.c, revision 8ea8ea35.

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

#include <array_length.h>
#include <stdint.h>
#include <math.h>
#include <math-barriers.h>
#include <libm-alias-float.h>
#include "math_config.h"
#include <math_uint128.h>
#include <s_sincosf_common.h>
#include <s_sincosf_data.h>

static float __attribute__ ((noinline))
as_cosf_database (float x, double r)
{
  uint32_t t = asuint (x);
  uint32_t ax = t & (~0u >> 1);
  for (unsigned i = 0; i < array_length (ST_COSF); i++)
    if (__glibc_unlikely (ST_COSF[i].uarg == ax))
      return ST_COSF[i].rh + ST_COSF[i].rl;
  return r;
}

static float __attribute__ ((noinline))
as_cosf_big (float x)
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
  double z = RBIG_SINCOSF (t, &ia);
  double z2 = z * z, z4 = z2 * z2;
  double aa = (A[0] + z2 * A[1]) + z4 * (A[2] + z2 * A[3]);
  double bb = (B[0] + z2 * B[1]) + z4 * (B[2] + z2 * B[3]);
  double s0 = TB_COSF[(ia + 8u) & 31], c0 = TB_COSF[ia & 31];
  double r = c0 + z * (aa * s0 - bb * (z * c0));
  uint32_t tr = asuint64 (r);
  uint64_t tail = (tr + 6) & (~UINT64_C(0) >> 36);
  if (__glibc_unlikely (tail <= 12))
    return as_cosf_database (x, r);
  return r;
}

#ifndef SECTION
#  define SECTION
#endif

#ifndef COSF
#  define COSF_FUNC __cosf
#else
#  define COSF_FUNC COSF
#endif

float SECTION
COSF_FUNC (float x)
{
  uint32_t t = asuint (x);
  uint32_t ax = t << 1;
  int ia;
  double z0 = x, z;
  if (__glibc_unlikely (ax > 0x99000000u || ax < 0x73000000))
    {
      if (__glibc_likely (ax < 0x73000000))
	{
	  if (__glibc_unlikely (ax < 0x66000000u))
	    {
	      if (__glibc_unlikely (ax == 0u))
		return 1.0f;
	      return 1.0f - 0x1p-25f;
	    }
	  return -0x1p-1f * x * x + 1.0f;
	}
      return as_cosf_big (x);
    }
  if (__glibc_likely (ax < 0x82a41896u))
    {
      if (__glibc_unlikely (ax == 0x812d97c8u))
	return as_cosf_database (x, 0.0);
      z = rltl0 (z0, &ia);
    }
  else
    {
      z = rltl (z0, &ia);
    }
  double z2 = z * z, z4 = z2 * z2;
  double aa = (A[0] + z2 * A[1]) + z4 * (A[2] + z2 * A[3]);
  double bb = (B[0] + z2 * B[1]) + z4 * (B[2] + z2 * B[3]);
  double c0 = TB_COSF[ia & 31], s0 = TB_COSF[(ia + 8) & 31];
  double r = c0 + aa * (z * s0) - bb * (z2 * c0);
  return r;
}

#ifndef COSF
libm_alias_float (__cos, cos)
#endif
