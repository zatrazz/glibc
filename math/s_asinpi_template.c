/* Return asin (x) / pi.
   Copyright (C) 2024 Free Software Foundation, Inc.
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
#include <math_private.h>

FLOAT
M_DECL_FUNC (__asinpi) (FLOAT x)
{
  FLOAT ax = M_FABS (x);
  if (__glibc_unlikely (isgreater (ax, M_LIT (1.0))))
    /* Domain error: asinpi(|x|>1).  */
    __set_errno (EDOM);
  else if (__glibc_unlikely (ax == M_TRUE_MIN))
    __set_errno (ERANGE);
  return M_SUF (__ieee754_asin) (x) / M_SUF (M_PI);;
}
declare_mgen_alias (__asinpi, asinpi);
