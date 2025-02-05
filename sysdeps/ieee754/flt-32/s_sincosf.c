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

#include <array_length.h>
#include <stdint.h>
#include <math.h>
#include <libm-alias-float.h>
#include "math_config.h"
#include "s_trig.h"

#define CORE_MATH_SUPPORT_ERRNO
#include <errno.h>

#ifndef SECTION
# define SECTION
#endif

#ifndef SINCOSF
# define SINCOSF_FUNC __sincosf
#else
# define SINCOSF_FUNC SINCOSF
#endif

typedef union {float f; uint32_t u;} b32u32_u;
typedef union {double f; uint64_t u;} b64u64_u;

static inline float
add_sign (float x, float rh, float rl)
{
  float sgn = copysignf (1.0f, x);
  return sgn * rh + sgn * rl;
}

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
  b32u32_u t = { .f = x };
  uint32_t ax = t.u & (~0u >> 1);
  for (unsigned i = 0; i < array_length (st); i++)
    if (__glibc_unlikely (st[i].uarg == ax))
      {
	*sout = add_sign (x, st[i].sh, st[i].sl);
	*cout = st[i].ch + st[i].cl;
      }
}

static const double b[] =
  {
    0x1.3bd3cc9be45dcp-6,  -0x1.03c1f081b0833p-14,
    0x1.55d3c6fc9ac1fp-24, -0x1.e1d3ff281b40dp-35};
static const double a[] =
  {
    0x1.921fb54442d17p-3,  -0x1.4abbce6256a39p-10,
    0x1.466bc5a518c16p-19, -0x1.32bdc61074ff6p-29};
static const double tb[] =
  {
     0x0p+0,                0x1.8f8b83c69a60bp-3,  0x1.87de2a6aea963p-2,
     0x1.1c73b39ae68c8p-1,  0x1.6a09e667f3bcdp-1,  0x1.a9b66290ea1a3p-1,
     0x1.d906bcf328d46p-1,  0x1.f6297cff75cbp-1,   0x1p+0,
     0x1.f6297cff75cbp-1,   0x1.d906bcf328d46p-1,  0x1.a9b66290ea1a3p-1,
     0x1.6a09e667f3bcdp-1,  0x1.1c73b39ae68c8p-1,  0x1.87de2a6aea963p-2,
     0x1.8f8b83c69a60bp-3,  0x0p+0,               -0x1.8f8b83c69a60bp-3,
    -0x1.87de2a6aea963p-2, -0x1.1c73b39ae68c8p-1, -0x1.6a09e667f3bcdp-1,
    -0x1.a9b66290ea1a3p-1, -0x1.d906bcf328d46p-1, -0x1.f6297cff75cbp-1,
    -0x1p+0,               -0x1.f6297cff75cbp-1,  -0x1.d906bcf328d46p-1,
    -0x1.a9b66290ea1a3p-1, -0x1.6a09e667f3bcdp-1, -0x1.1c73b39ae68c8p-1,
    -0x1.87de2a6aea963p-2, -0x1.8f8b83c69a60bp-3
  };

static void __attribute__ ((noinline))
as_sincosf_big (float x, float *sout, float *cout)
{
  b32u32_u t = { .f = x };
  uint32_t ax = t.u << 1;
  if (__glibc_unlikely (ax >= 0xffu << 24))
    { // nan or +-inf
      if (ax << 8)
	{
	  *sout = x + x;
	  *cout = x + x;
	  return; // nan
	}
      *sout = 0.0f / 0.0f; // to raise FE_INVALID
      *cout = __math_invalidf (0.0);
      return;
    }
  int ia;
  double z = rbig (t.u, &ia, 124);
  double z2 = z * z, z4 = z2 * z2;
  double aa = (a[0] + z2 * a[1]) + z4 * (a[2] + z2 * a[3]);
  double bb = (b[0] + z2 * b[1]) + z4 * (b[2] + z2 * b[3]);
  bb *= z;
  double s0 = tb[ia & 31], c0 = tb[(ia + 8) & 31];
  double s = s0 + z * (aa * c0 - bb * s0);
  double c = c0 - z * (aa * s0 + bb * c0);
  *sout = s;
  *cout = c;
  b64u64_u tr = { .f = c };
  uint64_t tail = (tr.u + 6) & (~(uint64_t) 0 >> 36);
  if (__glibc_unlikely (tail <= 12))
    return as_sincosf_database (x, sout, cout);
}

void
SECTION
SINCOSF_FUNC (float x, float *sout, float *cout)
{
  b32u32_u t = { .f = x };
  uint32_t ax = t.u << 1;
  int ia;
  double z0 = x, z;
  if (__glibc_likely (ax < 0x822d97c8u))
    {
      if (__glibc_unlikely (ax < 0x73000000))
	{
	  if (__glibc_unlikely (ax < 0x66000000u))
	    {
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
      z = rltl (z0, &ia, -0x1.b1bbead603d8bp-29, 0x1.45f306ep+2);
    }
  double z2 = z * z, z4 = z2 * z2;
  double aa = (a[0] + z2 * a[1]) + z4 * (a[2] + z2 * a[3]);
  double bb = (b[0] + z2 * b[1]) + z4 * (b[2] + z2 * b[3]);
  aa *= z;
  bb *= z2;
  double s0 = tb[ia & 31], c0 = tb[(ia + 8) & 31];
  double rs = s0 + (aa * c0 - bb * s0);
  double rc = c0 - (aa * s0 + bb * c0);
  *sout = rs;
  *cout = rc;
}

#ifndef SINCOSF
libm_alias_float (__sincos, sincos)
#endif
