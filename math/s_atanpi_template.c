/* Return atan (x) / pi.
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

#pragma GCC optimize ("O0")

FLOAT
M_DECL_FUNC (__atanpi) (FLOAT x)
{
  if (__glibc_unlikely (M_FABS (x) == M_TRUE_MIN))
    __set_errno (ERANGE);
  return M_SUF (__atan) (x) / M_SUF (M_PI);
}
declare_mgen_alias (__atanpi, atanpi);
