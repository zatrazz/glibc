/* Correctly-rounded sincos of binary32 value.

Copyright (c) 2024-2025 Alexei Sibidanov

The original version of this file was copied from the CORE-MATH
project (file src/binary32/sincos/sincosf.c, revision 8ea8ea35.

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

#include <array_length.h>
#include <math.h>
#include <libm-alias-float.h>
#include "math_config.h"
#include <math_uint128.h>
#include <s_sincosf_common.h>
#include <s_sincosf_data.h>

#ifndef SECTION
# define SECTION
#endif

#ifndef SINCOSF
# define SINCOSF_FUNC __sincosf
#else
# define SINCOSF_FUNC SINCOSF
#endif

static void __attribute__ ((noinline))
as_sincosf_database (float x, float *sout, float *cout)
{
  uint32_t t = asuint (x);
  uint32_t ax = t & (~0u >> 1);
  for (unsigned i = 0; i < array_length (ST_SINCOSF); i++)
    if (__glibc_unlikely (ST_SINCOSF[i].uarg == ax))
      {
	*sout = add_sign (x, ST_SINCOSF[i].sh, ST_SINCOSF[i].sl);
	*cout = ST_SINCOSF[i].ch + ST_SINCOSF[i].cl;
	break;
      }
}

static void __attribute__ ((noinline))
as_sincosf_big (float x, float *sout, float *cout)
{
  uint32_t t = asuint (x);
  uint32_t ax = t << 1;
  if (__glibc_unlikely (ax >= 0xffu << 24))
    { // nan or +-inf
      if (ax << 8)
	{
	  *sout = x + x;
	  *cout = x + x;
	  return; // nan
	}
      *sout = *cout = x - x;
      __math_invalidf (x + x);
      return;
    }
  int ia;
  double z = RBIG_SINCOSF (t, &ia);
  double z2 = z * z, z4 = z2 * z2;
  double aa = (A[0] + z2 * A[1]) + z4 * (A[2] + z2 * A[3]);
  double bb = (B[0] + z2 * B[1]) + z4 * (B[2] + z2 * B[3]);
  bb *= z;
  double s0 = TB[ia & 31], c0 = TB[(ia + 8u) & 31];
  double s = s0 + z * (aa * c0 - bb * s0);
  double c = c0 - z * (aa * s0 + bb * c0);
  *sout = s;
  *cout = c;
  uint64_t tr = asuint64 (c);
  uint64_t tail = (tr + 6) & (~UINT64_C(0) >> 36);
  if (__glibc_unlikely (tail <= 12))
    return as_sincosf_database (x, sout, cout);
}

void SECTION
SINCOSF_FUNC (float x, float *sout, float *cout)
{
  uint32_t ax = asuint (x) << 1;
  int ia;
  double z0 = x, z;
  if (__glibc_likely (ax < 0x822d97c8u))
    { // |x| < 0x1.2d97c8p+3
      if (__glibc_unlikely (ax < 0x73000000u))
	{ // |x| < 0x1p-12
	  if (__glibc_unlikely (ax < 0x66000000u))
	    { // |x| < 0x1p-25
	      if (__glibc_unlikely (ax == 0u))
		{
		  *sout = x;
		  *cout = 1.0f;
		}
	      else
		{
		  *sout = fmaf (-x, fabsf (x), x);
		  *cout = 1.0f - 0x1p-25f;
		}
	    }
	  else
	    {
	      *sout = (-0x1.555556p-3f * x) * (x * x) + x;
	      *cout = (-0x1p-1f * x) * x + 1.0f;
	    }
	  return;
	}
      if (__glibc_unlikely (ax == 0x812d97c8u))
	return as_sincosf_database (x, sout, cout);
      z = rltl0 (z0, &ia);
    }
  else
    {
      if (__glibc_unlikely (ax > 0x99000000u))
	return as_sincosf_big (x, sout, cout);
      if (__glibc_unlikely (ax == 0x8c333330u))
	return as_sincosf_database (x, sout, cout);
      z = rltl (z0, &ia);
    }
  double z2 = z * z, z4 = z2 * z2;
  double aa = (A[0] + z2 * A[1]) + z4 * (A[2] + z2 * A[3]);
  double bb = (B[0] + z2 * B[1]) + z4 * (B[2] + z2 * B[3]);
  aa *= z;
  bb *= z2;
  double s0 = TB[ia & 31], c0 = TB[(ia + 8) & 31];
  double rs = s0 + (aa * c0 - bb * s0);
  double rc = c0 - (aa * s0 + bb * c0);
  *sout = rs;
  *cout = rc;
}

#ifndef SINCOSF
libm_alias_float (__sincos, sincos)
#endif
