/* Common routines for sinf/cosf/tanf/sincosf.

Copyright (c) 2022-2026 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary32/tan/tanf.c, revision 59d21d7).

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

#ifndef _S_SINCOSF_COMMON_H
#define _S_SINCOSF_COMMON_H

#include <stdint.h>
#include "math_config.h"

static inline double
rltl0 (double x, int *q)
{
  double idh = 0x1.45f306dc9c883p+2 * x, id = roundeven_finite (idh);
  *q = asuint64 (0x1.8p52 + id);
  return idh - id;
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

static inline float
add_sign (float x, float rh, float rl)
{
  float sgn = copysignf (1.0f, x);
  return sgn * rh + sgn * rl;
}

extern double __tanf_rbig (uint32_t u, int *q) attribute_hidden;
#define RBIG_TANF __tanf_rbig
extern double __sincosf_rbig (uint32_t u, int *q) attribute_hidden;
#define RBIG_SINCOSF __sincosf_rbig

#endif
