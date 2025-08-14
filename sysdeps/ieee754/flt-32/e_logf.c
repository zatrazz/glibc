/* Correctly-rounded logarithm function for binary32 value.

Copyright (c) 2023-2024 Alexei Sibidanov and Paul Zimmermann.

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
#include "e_logf_data.h"

static __attribute__ ((noinline)) float
as_special (float x)
{
  uint32_t t = asuint (x);
  uint32_t ux = t;
  if (ux == 0u)
    return __math_divzerof (1);
  if (ux == 0x7f800000u)
    return x; // +inf
  uint32_t ax = ux << 1;
  if (ax == 0u)
    return __math_divzerof (1);
  if (ax > 0xff000000u)
    return x + x; // nan
  return __math_invalidf (x);
}

float
__logf (float x)
{
  uint32_t ux = asuint (x);
  if (__glibc_unlikely (ux < (1 << 23) || ux >= 0x7f800000u))
    {
      if (ux == 0 || ux >= 0x7f800000u)
	return as_special (x); // <=0, nan, inf
      // subnormal
      int n = stdc_leading_zeros (ux) - 8;
      ux <<= n;
      ux -= n << 23;
    }
  if (__glibc_unlikely (ux == 127u << 23))
    return 0.0f;
  uint32_t m = ux & ((1 << 23) - 1), j = (m + (1 << (23 - 7))) >> (23 - 6);
  int32_t e = ((int32_t) ux >> 23) - 127;
  double tz = asdouble (((uint64_t) m | ((int64_t) 1023 << 23)) << (52 - 23));
  double z = tz * TR[j] - 1, z2 = z * z;
  double r = ((e * 0x1.62e42fefa39efp-1 + TL[j]) + z * B[0])
	     + z2 * (B[1] + z * B[2]);
  float ub = r, lb = r + 0x1.f06p-33;
  if (__glibc_unlikely (ub != lb))
    {
      double f = z2
		 * ((C[0] + z * C[1])
		    + z2
			  * ((C[2] + z * C[3])
			     + z2 * (C[4] + z * C[5] + z2 * C[6])));
      if (__glibc_unlikely (fabsf (x - 1.0f) < 0x1p-10f))
	{
	  return z + f;
	}
      f -= 0x1.0ca86c3898dp-49 * e;
      f += z;
      f += TL[j] - TL[0];
      double el = e * 0x1.62e42fefa3ap-1;
      r = el + f;
      ub = r;
      tz = r;
      if (__glibc_unlikely (!(asuint64 (tz) & ((1u << 28) - 1u))))
	{
	  double dr = (el - r) + f;
	  r += dr * 64.0;
	  ub = r;
	}
    }
  return ub;
}
#ifndef __logf
strong_alias (__logf, __ieee754_logf)
libm_alias_finite (__ieee754_logf, __logf)
versioned_symbol (libm, __logf, logf, GLIBC_2_27);
libm_alias_float_other (__log, log)
#endif
