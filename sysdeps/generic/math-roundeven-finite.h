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

/* An architecture sets this to 1 and defines roundeven_finite and
   roundevenf_finite in its version of this header when it can do better
   than the generic definitions in math_config.h, which call the internal
   roundeven and roundevenf.  Both functions take a finite argument; the
   result for a non finite one is unspecified.  */
#define HAVE_ARCH_ROUNDEVEN_FINITE 0

#endif /* math-roundeven-finite.h */
