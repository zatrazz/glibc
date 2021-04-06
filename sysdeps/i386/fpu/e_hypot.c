/* Euclidean distance function.  Double/Binary64 i386 version.
   Copyright (C) 2021 Free Software Foundation, Inc.
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
#include <math_private.h>
#include <math-underflow.h>
#include <math-narrow-eval.h>
#include <math-svid-compat.h>
#include <libm-alias-finite.h>
#include <libm-alias-double.h>
#include <math_config.h>
#include <errno.h>

/* The i386 allows ot use the default excess of precision to optimize the
   hypot implementation, since internal multiplication and sqrt is carried
   with 80-bit FP type.  */
double
__hypot (double x, double y)
{
  if ((isinf (x) || isinf (y))
      && !issignaling (x) && !issignaling (y))
    return INFINITY;
  if (isnan (x) || isnan (y))
    return x + y;

  double r = math_narrow_eval (sqrt (x * x + y * y));
  math_check_force_underflow_nonneg (r);
  if (isinf (r))
    __set_errno (ERANGE);
  return r;
}
strong_alias (__hypot, __ieee754_hypot)
#if LIBM_SVID_COMPAT
versioned_symbol (libm, __hypot, hypot, GLIBC_2_34);
libm_alias_finite (__ieee754_hypot, __hypot)
libm_alias_double_other (__hypot, hypot)
#else
libm_alias_double (__hypot, hypot)
#endif
