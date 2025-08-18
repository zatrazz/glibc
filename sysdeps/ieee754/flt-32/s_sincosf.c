/* Compute sine and cosine of argument.
   Copyright (C) 2018-2025 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <stdint.h>
#include <math.h>
#include <math-barriers.h>
#include <libm-alias-float.h>
#include "s_sincosf.h"

static void __attribute__ ((noinline))
as_sincosf_database (float x, float *sout, float *cout)
{
  static const struct
  {
    union
    {
      float arg;
      uint32_t uarg;
    };
    float sh, sl, ch, cl;
  } st[] = {
    { { 0x1.33333p+13 }, -0x1.63f4bap-2, -0x1p-27, -0x1.e01216p-1, -0x1p-26 },
    { { 0x1.75b8a2p-1 }, 0x1.55688ap-1, -0x1p-26, 0x1.7d8e1ep-1, 0x1p-26 },
    { { 0x1.4f0654p+0 }, 0x1.ee836cp-1, -0x1p-26, 0x1.09558p-2, -0x1p-27 },
    { { 0x1.2d97c8p+3 }, -0x1.99bc5ap-26, -0x1p-51, -0x1p+0, 0x1p-25 },
    { { 0x1.2d97c8p+2 }, -0x1p+0, 0x1p-25, 0x1.99bc5cp-27, -0x1p-52 },
    { { 0x1.4555p+51 }, -0x1.b0ea44p-1, 0x1p-26, 0x1.115d7ep-1, -0x1p-26 },
    { { 0x1.48a858p+54 }, 0x1.beac8cp-1, 0x1p-26, 0x1.f48148p-2, 0x1p-27 },
    { { 0x1.3170fp+63 }, 0x1.5ac1eep-4, -0x1p-30, 0x1.fe2976p-1, 0x1p-26 },
    { { 0x1.2b9622p+67 }, -0x1.f983c2p-3, 0x1p-28, 0x1.f0285ep-1, -0x1p-26 },
  };
  uint32_t ax = asuint (x) & (~0u >> 1);
  for (unsigned i = 0; i < sizeof (st) / sizeof (st[0]); i++)
    if (__glibc_unlikely (st[i].uarg == ax))
      {
	*sout = add_sign (x, st[i].sh, st[i].sl);
	*cout = st[i].ch + st[i].cl;
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
  double z = rbig (t, &ia);
  double z2 = z * z, z4 = z2 * z2;
  double aa = (A[0] + z2 * A[1]) + z4 * (A[2] + z2 * A[3]);
  double bb = (B[0] + z2 * B[1]) + z4 * (B[2] + z2 * B[3]);
  bb *= z;
  double s0 = TB[ia & 31], c0 = TB[(ia + 8u) & 31];
  double s = s0 + z * (aa * c0 - bb * s0);
  double c = c0 - z * (aa * s0 + bb * c0);
  *sout = s;
  *cout = c;
  uint64_t tail = (asuint64 (c) + 6) & (~(uint64_t) 0 >> 36);
  if (__glibc_unlikely (tail <= 12))
    return as_sincosf_database (x, sout, cout);
}

#ifndef SECTION
#  define SECTION
#endif

#ifndef SINCOSF
#  define SINCOSF_FUNC __sincosf
#else
#  define SINCOSF_FUNC SINCOSF
#endif


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
		  /* We have underflow when |x| <= 0x1p-126 for rounding
		     towards zero, and when |x| < 0x1p-126 for rounding to
		     nearest or away from zero. In all cases this is when
		     |sout| < 0x1p-126. */
		  if (fabsf (*sout) < 0x1p-126)
		    __math_erange (x);
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
