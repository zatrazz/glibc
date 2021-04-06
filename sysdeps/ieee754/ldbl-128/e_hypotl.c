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
#define SQRT_EPS_DIV_2     L(0x1.6a09e667f3bcc908b2fb1366ea95p-57)
/* DBL_MIN / (sqrt (LDBL_EPSILON / 2.0))   */
#define DBL_MIN_THRESHOLD  L(0x1.6a09e667f3bcc908b2fb1366ea96p-16326)
/* eps (sqrt (LDBL_MIN))  */
#define SCALE              L(0x1p-8303)
/* 1 / eps (sqrt (LDBL_MIN)  */
#define INV_SCALE          L(0x1p+8303)
/* sqrt (LDBL_MAX)  */
#define SQRT_DBL_MAX       L(0x1.6a09e667f3bcc908b2fb1366ea95p+8191)
/* sqrt (LDBL_MIN)  */
#define SQRT_DBL_MIN       L(0x1p-8191)

_Float128
__ieee754_hypotl(_Float128 x, _Float128 y)
{
  if ((isinf (x) || isinf (y))
      && !issignaling (x) && !issignaling (y))
    return INFINITY;
  if (isnan (x) || isnan (y))
    return x + y;

  _Float128 ax = fabsl (x);
  _Float128 ay = fabsl (y);
  if (ay > ax)
    {
      _Float128 tmp = ax;
      ax = ay;
      ay = tmp;
    }

  /* Widely varying operands.  The DBL_MIN_THRESHOLD check is used to avoid
     an spurious underflow from the multiplication.  */
  if (ax >= DBL_MIN_THRESHOLD && ay <= ax * SQRT_EPS_DIV_2)
    return (ay == 0.0) ? ax : ax + LDBL_DENORM_MIN;

  _Float128 scale = SCALE;
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
    scale = 1.0;

  _Float128 h = sqrtl (ax * ax + ay * ay);

  _Float128 t1;
  _Float128 t2;
  if (h == 0.0)
    return h;
  if (h <= 2.0 * ay)
    {
      _Float128 delta = h - ay;
      t1 = ax * (2.0 * delta - ax);
      t2 = (delta - 2.0 * (ax - ay)) * delta;
    }
  else
    {
      _Float128 delta = h - ax;
      t1 = 2.0 * delta * (ax - 2 * ay);
      t2 = (4.0 * delta - ay) * ay + delta * delta;
    }
  h -= (t1 + t2) / (2.0 * h);
  h *= scale;
  math_check_force_underflow_nonneg (h);
  return h;
}
libm_alias_finite (__ieee754_hypotl, __hypotl)
