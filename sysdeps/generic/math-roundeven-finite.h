/* Round to nearest integer, ties to even, of a finite argument.  Generic.
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

#ifndef _MATH_ROUNDEVEN_FINITE_H
#define _MATH_ROUNDEVEN_FINITE_H 1

/* Round x to nearest integer value in floating-point format, rounding halfway
  cases to even.  If the input is non finite the result is unspecified.  */
static inline double
roundeven_finite (double x)
{
  if (!isfinite (x))
    __builtin_unreachable ();
  return roundeven (x);
}

static inline float
roundevenf_finite (float x)
{
  if (!isfinite (x))
    __builtin_unreachable ();
  return roundevenf (x);
}

#endif /* math-roundeven-finite.h */
