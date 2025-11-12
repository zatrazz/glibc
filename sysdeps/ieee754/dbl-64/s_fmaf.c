/* Compute x * y + z as ternary operation.
   Copyright (C) 2010-2025 Free Software Foundation, Inc.
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

#define NO_MATH_REDIRECT
#include <math.h>
#include <fenv.h>
#include <libm-alias-float.h>
#include <math-use-builtins.h>
#include "math_config.h"

#if !USE_FMAF_BUILTIN
# include <ieee754.h>
# include <math-barriers.h>
# include <fenv_private.h>

/* This implementation relies on double being more than twice as
   precise as float and uses rounding to odd in order to avoid problems
   with double rounding.
   See a paper by Boldo and Melquiond:
   http://www.lri.fr/~melquion/doc/08-tc.pdf  */

static __attribute_noinline__ float
fmaf_fallback (double xy, double z)
{
  fenv_t env;
  union ieee754_double u;

  libc_feholdexcept_setround (&env, FE_TOWARDZERO);

  /* Perform addition with round to odd.  */
  u.d = xy + (double) z;
  /* Ensure the addition is not scheduled after fetestexcept call.  */
  math_force_eval (u.d);

  /* Reset rounding mode and test for inexact simultaneously.  */
  int j = libc_feupdateenv_test (&env, FE_INEXACT) != 0;
  if ((u.ieee.mantissa1 & 1) == 0 && u.ieee.exponent != 0x7ff)
    u.ieee.mantissa1 |= j;

  /* And finally truncation with round to nearest.  */
  return u.d;
}
#endif

float
__fmaf (float x, float y, float z)
{
#if USE_FMAF_BUILTIN
  return __builtin_fmaf (x, y, z);
#else
  /* Multiplication is always exact.  */
  double xy = (double) x * (double) y;
  double result = xy + z;

  uint64_t u = asuint64 (result);
  /* Common case: The double precision result is fine. */
  if ((u & 0x1fffffff) != 0x10000000 ||          /* not a halfway case */
       (result - xy == z && result - z == xy) || /* exact */
        __fegetround () != FE_TONEAREST)         /* not round-to-nearest */
    {
      /* Underflow may not be raised correctly, example:
	 fmaf(0x1p-120f, 0x1p-120f, 0x1p-149f)  */
      int e = u >> MANTISSA_WIDTH & 0x7ff;
      if (__glibc_unlikely (e <= EXPONENT_BIAS - 126
			    && e >= EXPONENT_BIAS - 149))
	return fmaf_fallback (xy, z);
      return result;
    }

  /*
   * If result is inexact, and exactly halfway between two float values,
   * we need to adjust the low-order bit in the direction of the error.
   */
  double err;
  int neg = u >> 63;
  if (neg == (z > xy))
    err = xy - result + z;
  else
    err = z - result + xy;

  if (neg == (err < 0))
    u++;
  else
    u--;
  return asdouble (u);
#endif /* ! USE_FMAF_BUILTIN  */
}
#ifndef __fmaf
libm_alias_float (__fma, fma)
#endif
