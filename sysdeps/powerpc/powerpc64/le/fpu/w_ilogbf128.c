/* Get integer exponent of a floating-point value.
   Copyright (C) 2025 Free Software Foundation, Inc.
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

#include <math-type-macros-float128.h>
#include <math.h>
#include <math_private.h>
#include <stdbit.h>

#ifdef DEF_AS_LLOGBF128
# define DECL_NAME   __llogb
# define FUNC_NAME   llogb
# define RET_TYPE    long int
# define RET_LOGB0   FP_LLOGB0
# define RET_LOGBNAN FP_LLOGBNAN
# define RET_LOGMAX  LONG_MAX
# define RET_INVALID __math_invalid_li
#else
# define DECL_NAME   __ilogb
# define FUNC_NAME   ilogb
# define RET_TYPE    int
# define RET_LOGB0   FP_ILOGB0
# define RET_LOGBNAN FP_ILOGBNAN
# define RET_LOGMAX  INT_MAX
# define RET_INVALID __math_invalid_i
#endif
#define __IMPL_NAME(x,y) x ## _ ## y
#define _IMPL_NAME(x,y)  __IMPL_NAME(x,y)
#define IMPL_NAME        _IMPL_NAME(FUNC_NAME, impl)
#include <w_ilogbf128-impl.h>

RET_TYPE
M_DECL_FUNC (DECL_NAME) (FLOAT x)
{
  return IMPL_NAME (x);
}
declare_mgen_alias (DECL_NAME, FUNC_NAME)
