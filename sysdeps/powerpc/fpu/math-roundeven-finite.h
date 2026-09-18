/* Round to nearest integer, ties to even, of a finite argument.  PowerPC.
   Copyright (C) 2026 Free Software Foundation, Inc.
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

#ifndef _MATH_ROUNDEVEN_FINITE_POWERPC_H
#define _MATH_ROUNDEVEN_FINITE_POWERPC_H 1

#ifdef _ARCH_PWR6
# include <math.h>
# include <stdbit.h>
# include <stdint.h>

/* ISA 2.03 provides frin/round() and cntlzw/ctznll().  */

static inline double
roundeven_finite (double x)
{
  double y = round (x);
  if (fabs (x - y) == 0.5)
    {
      union { double f; uint64_t i; } u = {y};
      union { double f; uint64_t i; } v = {y - copysign (1.0, x)};
      if (stdc_trailing_zeros (v.i) > stdc_trailing_zeros (u.i))
        y = v.f;
    }
  return y;
}

static inline float
roundevenf_finite (float x)
{
  float y = roundf (x);
  if (fabsf (x - y) == 0.5f)
    {
      union { float f; uint32_t i; } u = {y};
      union { float f; uint32_t i; } v = {y - copysignf (1.0f, x)};
      if (stdc_trailing_zeros (v.i) > stdc_trailing_zeros (u.i))
        y = v.f;
    }
  return y;
}
#else
# include_next <math-roundeven-finite.h>
#endif

#endif /* math-roundeven-finite.h */
