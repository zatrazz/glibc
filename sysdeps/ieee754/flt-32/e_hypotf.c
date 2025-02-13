/* Euclidean distance function.  Float/Binary32 version.
   Copyright (C) 2012-2025 Free Software Foundation, Inc.
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
#include <libm-alias-finite.h>
#include <libm-alias-float.h>
#include <math-svid-compat.h>
#include <math.h>
#include "math_config.h"

float
__hypotf (float x, float y)
{
  float ax = fabsf (x), ay = fabsf (y);
  uint32_t tx = asuint (ax), ty = asuint (ay);
  if (__glibc_unlikely (tx >= (0xffu << 23) || ty >= (0xffu << 23)))
    {
      if ((tx == (0xffu << 23) || ty == (0xffu << 23)) // x == inf or x == inf
         && !issignaling (x) && !issignaling (y))
       return INFINITY;
      return ax + ay;
    }
  float at = ax > ay ? ax : ay;
  ay = ax < ay ? ax : ay;
  double xd = at, yd = ay, x2 = xd * xd, y2 = yd * yd, r2 = x2 + y2;
  if (__glibc_unlikely (yd < xd * 0x1.fffffep-13))
    return fmaf (0x1p-13f, ay, at);
  double r = sqrt (r2);
  uint64_t t = asuint64 (r);
  float c = r;
  if (t > UINT64_C(0x47efffffe0000000))
    {
      if (c > 0x1.fffffep127f)
       errno = ERANGE;
      return c;
    }
  if (__glibc_likely ((t + 1) & 0xfffffff) > 2)
    return c;
  double cd = c;
  if ((cd * cd - x2) - y2 == 0.0)
    return c;
  double ir2 = 0.5 / r2, dr2 = (x2 - r2) + y2;
  double rs = r * ir2, dz = dr2 - fma (r, r, -r2), dr = rs * dz;
  double rh = r + dr, rl = dr + (r - rh);
  t = asuint64 (rh);
  if (__glibc_likely ((t & 0xfffffff) == 0))
    {
      if (rl > 0.0)
       t += 1;
      if (rl < 0.0)
       t -= 1;
    }
  return asdouble (t);
}
strong_alias (__hypotf, __ieee754_hypotf)
#if LIBM_SVID_COMPAT
versioned_symbol (libm, __hypotf, hypotf, GLIBC_2_35);
libm_alias_float_other (__hypot, hypot)
#else
libm_alias_float (__hypot, hypot)
#endif
libm_alias_finite (__ieee754_hypotf, __hypotf)
