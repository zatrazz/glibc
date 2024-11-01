/* Correctly-rounded reciprocal square root of binary32 value.

Copyright (c) 2022-2023 Alexei Sibidanov.

This file is part of the CORE-MATH project
project (file src/binary32/rsqrt/rsqrtf.c, revision 89fd12b).

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
#include <libm-alias-float.h>
#include "math_config.h"

float
__rsqrtf (float x)
{
  double xd = x;
  uint32_t ix = asuint (x);
  if (__glibc_unlikely (ix >= 0xff << 23 || ix == 0))
    {
      if (!(ix << 1))
	return __math_divzerof (ix >> 31);
      if (ix >> 31)
	{
	  ix &= ~0u >> 1;
	  if (ix > 0xff << 23)
	    return x + x; // nan
	  return __math_invalidf (x);
	}
      if (!(ix << 9))
	return 0.0f;
      return x + x; // nan
    }
  unsigned int m = ix << 8;
  if (__glibc_unlikely (ix == 0x2f7e2au
			|| m == 0xbdf8a800u
			|| m == 0x55b7bd00u))
    {
      if (ix != 0x0055b7bdu)
	{
	  unsigned int e = ix >> 23;
	  unsigned int k = 1;
	  if (ix == 0x2f7e2au)
	    e = -1;
	  if (m == 0x55b7bd00u)
	    k = 0;
	  static const uint32_t tb[] = { 0x000c1740u, 0x005222e0u };
	  uint32_t r = tb[k];
	  uint32_t dr;
	  e = (512 - e) / 2 - 578;
	  r |= e << 23;
	  dr = (e - 25) << 23;
	  return asfloat (r) - asfloat (dr);
	}
    }
  return (1.0 / xd) * sqrt (xd);
}
libm_alias_float (__rsqrt, rsqrt)
