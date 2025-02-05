/* Correctly-rounded cosine of binary32 value.

Copyright (c) 2022-2025 Alexei Sibidanov.

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

#include <stdint.h>
#include <math.h>
#include <math-barriers.h>
#include <libm-alias-float.h>
#include "math_config.h"
#include "s_trig.h"

#define CORE_MATH_SUPPORT_ERRNO
#include <errno.h>

#ifndef SECTION
# define SECTION
#endif

#ifndef COSF
# define COSF_FUNC __cosf
#else
# define COSF_FUNC COSF
#endif

static float __attribute__ ((noinline))
as_cosf_database (float x, double r)
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
    { { 0x1.2d97c8p+2 }, 0x1.99bc5cp-27, -0x1p-52 },
    { { 0x1.4555p+51 }, 0x1.115d7ep-1, -0x1p-26 },
    { { 0x1.48a858p+54 }, 0x1.f48148p-2, 0x1p-27 },
    { { 0x1.3170fp+63 }, 0x1.fe2976p-1, 0x1p-26 },
    { { 0x1.2b9622p+67 }, 0x1.f0285ep-1, -0x1p-26 },
  };
  uint32_t t = asuint (x);
  uint32_t ax = t & (~0u >> 1);
  for (unsigned i = 0; i < sizeof (st) / sizeof (st[0]); i++)
    if (__glibc_unlikely (st[i].uarg == ax))
      return st[i].rh + st[i].rl;
  return r;
}

static const double b[] =
  {
    0x1.3bd3cc9be45dcp-6, -0x1.03c1f081b0833p-14,
    0x1.55d3c6fc9ac1fp-24, -0x1.e1d3ff281b40dp-35
  };
static const double a[] =
  {
    0x1.921fb54442d17p-3, -0x1.4abbce6256a39p-10,
    0x1.466bc5a518c16p-19, -0x1.32bdc61074ff6p-29
  };
static const double tb[] =
  {
     0x1p+0,                0x1.f6297cff75cbp-1,   0x1.d906bcf328d46p-1,
     0x1.a9b66290ea1a3p-1,  0x1.6a09e667f3bcdp-1,  0x1.1c73b39ae68c8p-1,
     0x1.87de2a6aea963p-2,  0x1.8f8b83c69a60bp-3,  0x0p+0,
    -0x1.8f8b83c69a60bp-3, -0x1.87de2a6aea963p-2, -0x1.1c73b39ae68c8p-1,
    -0x1.6a09e667f3bcdp-1, -0x1.a9b66290ea1a3p-1, -0x1.d906bcf328d46p-1,
    -0x1.f6297cff75cbp-1,  -0x1p+0,               -0x1.f6297cff75cbp-1,
    -0x1.d906bcf328d46p-1, -0x1.a9b66290ea1a3p-1, -0x1.6a09e667f3bcdp-1,
    -0x1.1c73b39ae68c8p-1, -0x1.87de2a6aea963p-2, -0x1.8f8b83c69a60bp-3,
     0x0p+0,                0x1.8f8b83c69a60bp-3,  0x1.87de2a6aea963p-2,
     0x1.1c73b39ae68c8p-1,  0x1.6a09e667f3bcdp-1,  0x1.a9b66290ea1a3p-1,
     0x1.d906bcf328d46p-1,  0x1.f6297cff75cbp-1
  };

static float __attribute__ ((noinline))
as_cosf_big (float x)
{
  uint32_t t = asuint (x);
  uint32_t ax = t << 1;
  if (__glibc_unlikely (ax >= 0xffu << 24))
    { // nan or +-inf
      if (ax << 8)
	return x + x; // nan
      return __math_invalidf (0);
    }
  int ia;
  double z = rbig (t, &ia, 124);
  double z2 = z * z, z4 = z2 * z2;
  double aa = (a[0] + z2 * a[1]) + z4 * (a[2] + z2 * a[3]);
  double bb = (b[0] + z2 * b[1]) + z4 * (b[2] + z2 * b[3]);
  double s0 = tb[(ia + 8) & 31], c0 = tb[ia & 31];
  double r = c0 + z * (aa * s0 - bb * (z * c0));
  uint64_t tr = asuint64 (r);
  uint64_t tail = (tr + 6) & (~UINT64_C(0) >> 36);
  if (__glibc_unlikely (tail <= 12))
    return as_cosf_database (x, r);
  return r;
}

float
SECTION
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
      z = rltl (z0, &ia, -0x1.b1bbead603d8bp-29, 0x1.45f306ep+2);
    }
  double z2 = z * z, z4 = z2 * z2;
  double aa = (a[0] + z2 * a[1]) + z4 * (a[2] + z2 * a[3]);
  double bb = (b[0] + z2 * b[1]) + z4 * (b[2] + z2 * b[3]);
  double c0 = tb[ia & 31], s0 = tb[(ia + 8) & 31];
  double r = c0 + aa * (z * s0) - bb * (z2 * c0);
  return r;
}

#ifndef COSF
libm_alias_float (__cos, cos)
#endif
