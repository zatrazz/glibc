/* Euclidean distance function.  Long Double/Binary96 version.
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

/* This implementation is based on the 'An Improved Algorithm for
   hypot(a,b)' by Carlos F. Borges [1] using the MyHypot3 with the
   following changes:

   - Handle qNaN and sNaN.
   - Tune the 'widely varying operands' to avoid spurious underflow
     due the multiplication and fix the return value for upwards
     rounding mode.
   - Handle required underflow exception for denormal results.

   [1] https://arxiv.org/pdf/1904.09481.pdf  */

#include <math.h>
#include <math_private.h>
#include <math-underflow.h>
#include <libm-alias-finite.h>

/* sqrt (LDBL_EPSILON / 2.0)  */
#define SQRT_EPS_DIV_2     0x8p-35L
/* DBL_MIN / (sqrt (LDBL_EPSILON / 2.0))   */
#define DBL_MIN_THRESHOLD  0x8p-16353L
/* eps (sqrt (LDBL_MIN))  */
#define SCALE              0x8p-8257L
/* 1 / eps (sqrt (LDBL_MIN)  */
#define INV_SCALE          0x8p+8251L
/* sqrt (LDBL_MAX)  */
#define SQRT_DBL_MAX       0xb.504f333f9de6484p+8188L
/* sqrt (LDBL_MIN)  */
#define SQRT_DBL_MIN       0x8p-8194L

long double
__ieee754_hypotl (long double x, long double y)
{
  if ((isinf (x) || isinf (y))
      && !issignaling (x) && !issignaling (y))
    return INFINITY;
  if (isnan (x) || isnan (y))
    return x + y;

  long double ax = fabsl (x);
  long double ay = fabsl (y);
  if (ay > ax)
    {
      long double tmp = ax;
      ax = ay;
      ay = tmp;
    }

  /* Widely varying operands.  The DBL_MIN_THRESHOLD check is used to avoid
     an spurious underflow from the multiplication.  */
  if (ax >= DBL_MIN_THRESHOLD && ay <= ax * SQRT_EPS_DIV_2)
    return (ay == 0.0) ? ax : ax + LDBL_DENORM_MIN;

  long double scale = SCALE;
  if (ax > SQRT_DBL_MAX)
    {
      ax *= scale;
      ay *= scale;
      scale = INV_SCALE;
    }
  else if (ay < SQRT_DBL_MIN)
    {
      ax /= scale;
      ay /= scale;
    }
  else
    scale = 1.0L;

  long double h = sqrtl (ax * ax + ay * ay);

  long double t1;
  long double t2;
  if (h == 0.0L)
    return h;
  if (h <= 2.0L * ay)
    {
      long double delta = h - ay;
      t1 = ax * (2.0L * delta - ax);
      t2 = (delta - 2.0L * (ax - ay)) * delta;
    }
  else
    {
      long double delta = h - ax;
      t1 = 2.0L * delta * (ax - 2.0L * ay);
      t2 = (4.0L * delta - ay) * ay + delta * delta;
    }
  h -= (t1 + t2) / (2.0L * h);
  h *= scale;
  math_check_force_underflow_nonneg (h);
  return h;
}
libm_alias_finite (__ieee754_hypotl, __hypotl)
