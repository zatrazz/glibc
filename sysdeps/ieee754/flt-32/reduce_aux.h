/* Auxiliary routine for the Bessel functions (j0f, y0f, j1f, y1f).
   Copyright (C) 2021-2025 Free Software Foundation, Inc.
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

#ifndef _MATH_REDUCE_AUX_H
#define _MATH_REDUCE_AUX_H

#include <math.h>
#include <math_private.h>
#include "math_config.h"

/* 2PI * 2^-64.  */
static const double pi63 = 0x1.921FB54442D18p-62;

/* Table with 4/PI to 192 bit precision.  */
extern const uint32_t __inv_pio4[] attribute_hidden;

/* Reduce the range of XI to a multiple of PI/2 using fast integer arithmetic.
   XI is a reinterpreted float and must be >= 2.0f (the sign bit is ignored).
   Return the modulo between -PI/4 and PI/4 and store the quadrant in NP.
   Reduction uses a table of 4/PI with 192 bits of precision.  A 32x96->128 bit
   multiply computes the exact 2.62-bit fixed-point modulo.  Since the result
   can have at most 29 leading zeros after the binary point, the double
   precision result is accurate to 33 bits.  */
static inline double
reduce_large (uint32_t xi, int *np)
{
  const uint32_t *arr = &__inv_pio4[(xi >> 26) & 15];
  int shift = (xi >> 23) & 7;
  uint64_t n, res0, res1, res2;

  xi = (xi & 0xffffff) | 0x800000;
  xi <<= shift;

  res0 = xi * arr[0];
  res1 = (uint64_t)xi * arr[4];
  res2 = (uint64_t)xi * arr[8];
  res0 = (res2 >> 32) | (res0 << 32);
  res0 += res1;

  n = (res0 + (1ULL << 61)) >> 62;
  res0 -= n << 62;
  double x = (int64_t)res0;
  *np = n;
  return x * pi63;
}

/* Return h and update n such that:
   Now x - pi/4 - alpha = h + n*pi/2 mod (2*pi).  */
static inline double
reduce_aux (float x, int *n, double alpha)
{
  double h;
  h = reduce_large (asuint (x), n);
  /* Now |x| = h+n*pi/2 mod 2*pi.  */
  /* Recover sign.  */
  if (x < 0)
    {
      h = -h;
      *n = -*n;
    }
  /* Subtract pi/4.  */
  double piover2 = 0xc.90fdaa22168cp-3;
  if (h >= 0)
    h -= piover2 / 2;
  else
    {
      h += piover2 / 2;
      (*n) --;
    }
  /* Subtract alpha and reduce if needed mod pi/2.  */
  h -= alpha;
  if (h > piover2)
    {
      h -= piover2;
      (*n) ++;
    }
  else if (h < -piover2)
    {
      h += piover2;
      (*n) --;
    }
  return h;
}

#endif
