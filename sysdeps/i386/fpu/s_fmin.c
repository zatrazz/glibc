/* Return maximum numeric value of X and Y.  i386 version.
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

#include <math.h>
#include <libm-alias-double.h>

double
__fmin (double x, double y)
{
  /* gcc optimizes isunordered to 'fucomi' (i686) or 'fucom' (i486), and both
     already raises FE_INVALID if an operand is a sNaN.  So there is no need
     to check for issignaling.  */
  if (__glibc_likely (!isunordered (x, y)))
    {
      if (__glibc_unlikely (x == y))
	return signbit (x) ? x : y;
      return x > y ? y : x;
    }
  return isnan (y) ? x : y;
}
libm_alias_double (__fmin, fmin)
