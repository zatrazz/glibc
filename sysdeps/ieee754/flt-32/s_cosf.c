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
#include <s_sincosf_data.h>

static double __attribute__ ((noinline))
rbig (uint32_t u, int *q)
{
  int e = (u >> 23) & 0xff, i;
  uint64_t m = (u & (~0u >> 9)) | 1 << 23;
  u128 p0 = u128_mul (u128_from_u64 (m), u128_from_u64 (IPI[0]));
  u128 p1 = u128_mul (u128_from_u64 (m), u128_from_u64 (IPI[1]));
  p1 = u128_add (p1, u128_rshift (p0, 64));
  u128 p2 = u128_mul (u128_from_u64 (m), u128_from_u64 (IPI[2]));
  p2 = u128_add (p2, u128_rshift (p1, 64));
  u128 p3 = u128_mul (u128_from_u64 (m), u128_from_u64 (IPI[3]));
  p3 = u128_add (p3, u128_rshift (p2, 64));
  uint64_t p3h = u128_high (p3), p3l = u128_low (p3), p2l = u128_low (p2),
	   p1l = u128_low (p1);
  int64_t A;
  int k = e - 124, s = k - 23;
  /* in cr_sinf(), rbig() is called in the case 127+28 <= e < 0xff
     thus 155 <= e <= 254, which yields 28 <= k <= 127 and 5 <= s <= 104 */
  if (s < 64)
    {
      i = p3h << s | p3l >> (64 - s);
      A = p3l << s | p2l >> (64 - s);
    }
  else if (s == 64)
    {
      i = p3l;
      A = p2l;
    }
  else
    { /* s > 64 */
      i = p3l << (s - 64) | p2l >> (128 - s);
      A = p2l << (s - 64) | p1l >> (128 - s);
    }
  int sgn = u;
  sgn >>= 31;
  int64_t sm = A >> 63;
  i -= sm;
  double z = (A ^ sgn) * 0x1p-64;
  i = (i ^ sgn) - sgn;
  *q = i;
  return z;
}

static inline double
rltl (float z, int *q)
{
  double x = z;
  double idl = -0x1.b1bbead603d8bp-29 * x, idh = 0x1.45f306ep+2 * x,
	 id = roundeven_finite (idh);
  *q = asuint64 (0x1.8p52 + id);
  return (idh - id) + idl;
}

static inline double
rltl0 (double x, int *q)
{
  double idh = 0x1.45f306dc9c883p+2 * x, id = roundeven_finite (idh);
  *q = asuint64 (0x1.8p52 + id);
  return idh - id;
}

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
  double z = rbig (t, &ia);
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
