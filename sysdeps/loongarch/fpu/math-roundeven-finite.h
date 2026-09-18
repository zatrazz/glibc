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

/* The result is exact for |x| < 2^63. FTINTRNE also raises the inexact
   the current usage tolerates it.  */
static inline double
roundeven_finite (double x)
{
  double r;
  __asm__ ("ftintrne.l.d %0, %1\n\t"
	   "ffint.d.l %0, %0"
	   : "=f" (r) : "f" (x));
  return r;
}

static inline float
roundevenf_finite (float x)
{
  float r;
  __asm__ ("ftintrne.l.s %0, %1\n\t"
	   "ffint.s.l %0, %0"
	   : "=f" (r) : "f" (x));
  return r;
}

#endif
