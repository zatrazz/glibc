/* Round to nearest integer, ties to even, of a finite argument.  LoongArch.
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

/* GCC does not inline roundeven on LoongArch, while the round-to-nearest-
   even conversion to a 64-bit integer and back is two instructions that do
   not depend on the rounding mode.  The result is exact for |x| < 2^63,
   well beyond what the callers pass.  FTINTRNE raises the inexact exception
   for a non integer argument, which the callers tolerate: their result is
   inexact anyway.  */
#define HAVE_ARCH_ROUNDEVEN_FINITE 1

static inline double
roundeven_finite (double x)
{
  double r;
  __asm__ ("ftintrne.l.d %0, %1\n\tffint.d.l %0, %0" : "=f" (r) : "f" (x));
  return r;
}

static inline float
roundevenf_finite (float x)
{
  float r;
  __asm__ ("ftintrne.l.s %0, %1\n\tffint.s.l %0, %0" : "=f" (r) : "f" (x));
  return r;
}

#endif /* math-roundeven-finite.h */
