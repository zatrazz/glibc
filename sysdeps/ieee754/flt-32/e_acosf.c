/* Correctly-rounded arc-cosine function for binary32 value.

Copyright (c) 2023-2024 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary32/acos/acosf.c, revision 56dd347).

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
#include <math_private.h>
#include <libm-alias-finite.h>
#include <math-barriers.h>
#include "math_config.h"
#include "e_asincosf_data.h"

static __attribute__ ((noinline)) float
as_special (float x)
{
  const float pih = 0x1.921fb6p+1;
  const float pil = -0x1p-24f;
  uint32_t t = asuint (x);
  if (t == (0x7fu << 23))
    return 0.0f; /* x=1 */
  if (t == (0x17fu << 23))
    return pih + pil; /* x=-1 */
  uint32_t ax = t << 1;
  if (ax > (0xffu << 24))
    return x + x; /* nan */
  return __math_invalidf (0.0);
}

static inline double
poly12 (double z, const double *c)
{
  double z2 = z * z, z4 = z2 * z2;
  double c0 = c[0] + z * c[1];
  double c2 = c[2] + z * c[3];
  double c4 = c[4] + z * c[5];
  double c6 = c[6] + z * c[7];
  double c8 = c[8] + z * c[9];
  double c10 = c[10] + z * c[11];
  c0 += c2 * z2;
  c4 += c6 * z2;
  c8 += z2 * c10;
  c0 += z4 * (c4 + z4 * c8);
  return c0;
}

float
__ieee754_acosf (float x)
{
  double pi2 = 0x1.921fb54442d18p+0;
  static const double o[] = { 0, 0x1.921fb54442d18p+1 };
  double xs = x;
  double r;
  uint32_t t = asuint (x);
  uint32_t ax = t << 1;
  if (__glibc_unlikely (ax >= 0x7f<<24))
    return as_special (x);
  if (__glibc_likely (ax < 0x7ec2a1dcu)) /* |x| < 0x1.c2a1dcp-1 */
    {
      /* Avoid spurious underflow exception.  */
      if (__glibc_unlikely (ax <= 0x40000000u)) /* |x| < 2^-63 */
	/* GCC <= 11 wrongly assumes the rounding is to nearest and
	   performs a constant folding here:
	   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57245 */
	return math_opt_barrier (pi2);
      double z = xs;
      double z2 = z * z;
      double z4 = z2 * z2;
      double z8 = z4 * z4;
      double z16 = z8 * z8;
      r = z * ((((__asincosf_b[0] + z2 * __asincosf_b[1]) + z4
		 * (__asincosf_b[2] + z2 * __asincosf_b[3]))
		+ z8 * ((__asincosf_b[4] + z2 * __asincosf_b[5]) + z4
			* (__asincosf_b[6] + z2 * __asincosf_b[7])))
	       + z16 * (((__asincosf_b[8] + z2 * __asincosf_b[9]) + z4
			 * (__asincosf_b[10] + z2 * __asincosf_b[11]))
			+ z8 * ((__asincosf_b[12] + z2 * __asincosf_b[13])
				+ z4 * (__asincosf_b[14] + z2
					* __asincosf_b[15]))));
      float ub = 0x1.921fb54574191p+0 - r;
      float lb = 0x1.921fb543118ap+0 - r;
      if (ub == lb)
	return ub;
    }
  /* accurate path  */
  if (ax < (0x7eu << 24))
    {
      if (t == 0x328885a3u)
	return 0x1.921fb6p+0f + 0x1p-25;
      if (t == 0x39826222u)
	return 0x1.920f6ap+0f + 0x1p-25;
      double x2 = xs * xs;
      r = (pi2 - xs) - (xs * x2) * poly12 (x2, __asincosf_c0);
    }
  else
    {
      double bx = fabs (xs);
      double z = 1.0 - bx;
      double s = copysign (sqrt (z), xs);
      r = o[t >> 31] + s * poly12 (z, __asincosf_c1);
    }
  return r;
}
libm_alias_finite (__ieee754_acosf, __acosf)
