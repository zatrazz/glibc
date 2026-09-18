/* Round to nearest integer, ties to even, for range reductions.  Generic.
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

/* Return X rounded to the nearest integer value in floating-point format,
   with halfway cases rounded to even, independently of the rounding mode.
   They are meant for range reductionsi, and X must be in [-2^63, 2^63), the
   sign of a zero result is unspecified, and the inexact exception may be
   raised when X is not an integer.
   An architecture can then implement them with a conversion to a 64-bit
   integer and back.  */
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

#endif
