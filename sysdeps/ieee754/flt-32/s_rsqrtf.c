/* Correctly rounded reciprocal square root of binary32 value.
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

#include <errno.h>
#include <math.h>
#include <libm-alias-float.h>

float
__rsqrtf (float x)
{
  /* For +-0, x < 0, +Inf and NaN, the square root and the division already
     give the expected result and exception; only errno needs to be set.  */
  if (islessequal (x, 0.0f))
    __set_errno (x < 0 ? EDOM : ERANGE);
  /* Evaluating 1/sqrt(x) in double precision and rounding it to float gives
     the correctly rounded result for every binary32 input in all rounding
     modes, which was checked exhaustively.  The result is never subnormal
     and never overflows.  */
  return 1.0 / sqrt ((double) x);
}
libm_alias_float (__rsqrt, rsqrt)
