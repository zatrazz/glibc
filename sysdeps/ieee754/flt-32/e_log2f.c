/* Correctly-rounded binary logarithm function for binary32 value.

Copyright (c) 2022 Alexei Sibidanov.

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
#include <stdbit.h>
#include <libm-alias-finite.h>
#include <libm-alias-float.h>
#include "math_config.h"
#include "e_log2f_data.h"

float
__log2f (float x)
{
  uint32_t ux = asuint (x);
  uint64_t m = ux & (~0u >> 9);
  m <<= 52 - 23;
  int e = (ux >> 23) - 0x7f;
  if (__glibc_unlikely (ux < 1u << 23 || ux >= 0xffu << 23))
    {
      if (ux == 0 || ux == (1u << 31))
	// x = +/-0
	return __math_divzerof (1);
      uint32_t inf_or_nan = ((ux >> 23) & 0xff) == 0xff,
	       nan = inf_or_nan && (ux << 9);
      if (ux >> 31 && !nan)
	return __math_invalidf (x);
      if (inf_or_nan)
	return x + x;
      // subnormal
      int nz = stdc_leading_zeros (m);
      m <<= nz - 11;
      m &= ~(uint64_t) 0 >> 12;
      e -= nz - 12;
    }
  if (__glibc_unlikely (!m))
    return e;
  int j = (m + ((int64_t) 1 << (52 - 8))) >> (52 - 7), k = j > 53;
  e += k;
  double xd = asdouble (m | (uint64_t) 0x3ff << 52);
  double z = fma (xd, IX[j], -1.0); // z is exact
  double z2 = z * z;
  double c0 = C[0] + z * C[1];
  double c2 = C[2] + z * C[3];
  double c4 = C[4] + z * C[5];
  c0 += z2 * (c2 + z2 * c4);
  const double iln2 = 0x1.71547652b82fep+0;
  return (z * iln2) * c0 + (e - LIX[j] * iln2);
}
#ifndef __log2f
strong_alias (__log2f, __ieee754_log2f)
libm_alias_finite (__ieee754_log2f, __log2f)
versioned_symbol (libm, __log2f, log2f, GLIBC_2_27);
libm_alias_float_other (__log2, log2)
#endif
