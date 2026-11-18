/* Routines shared by the correctly-rounded binary64 sin and cos.

Copyright (c) 2026 Paul Zimmermann and Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/sin/sin.c, revision a3fe413f).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* References:
   [1] Handbook of Floating-Point Arithmetic (2nd edition),
   Muller, Jean-Michel and Brunie, Nicolas and de Dinechin, Florent and
   Jeannerod, Claude-Pierre and Joldes, Mioara and Lefèvre, Vincent and
   Melquiond, Guillaume and Revol, Nathalie and Torres, Serge,
   Birkhäuser, 2018.
   [2] Computing hard-to-round cases of sin, cos, tan in double precision,
   Vincent Lefèvre, Tue Ly, Paul Zimmermann,
   ARITH 2026 - 33rd IEEE International Symposium on Computer Arithmetic,
   2026.  */

#ifndef _S_SIN_COMMON_H
#define _S_SIN_COMMON_H

#include <math.h>
#include <math_uint128.h>
#include <stdint.h>
#include <ddcoremath.h>
#include "math_config.h"
#include "s_sincos_data.h"

/* Return the double-double product (xh + xl) * (ch + cl) with the low part
   in L.  The operand order differs from muldd2 in ddcoremath.h, which
   changes the FMA contraction of the low part.  */
static inline double
muldd (double xh, double xl, double ch, double cl, double *l)
{
  double ahhh = xh * ch;
  *l = (xh * cl + xl * ch) + fma (xh, ch, -ahhh);
  return ahhh;
}

/* Accurate sin path for |x| < 2^-16.  */
static inline double
sin_small_accurate (double x)
{
  /* x + (c3h+c3l)*x^3 + c5*x^5 approximates sin(x) on [0,2^-16] with
     relative error < 2^-112.743, cf sinsmall_acc.sollya, where c3h = c[0],
     c3l = c[1] and c5 = c[2].  */
  static const double c[] =
    {
      -0x1.5555555555555p-3, -0x1.55554b00de7e8p-57, 0x1.111111110848p-7
    };
  double h, l, t, x2h = x * x, x2l = fma (x, x, -x2h);
  /* Relative error less than ulp(c5*x^4)/ulp(x) ~ 2^-123.  */
  h = c[2] * x2h;
  /* Relative error less than ulp(c3l*x^2)/ulp(x) ~ 2^-141.  */
  h += c[1];
  h = fasttwosum (c[0], h, &l);
  h = muldd (h, l, x2h, x2l, &l);
  h = muldd (h, l, x, 0, &l);
  h = fasttwosum (x, h, &t);
  l += t;
  return h + l;
}

/* Argument reduction for x >= 2^31 (x is assumed to be positive): return k
   and r such that x/(2pi) mod 1 = k/2^15 + r + s with 0 <= r < 2^-15 and
   0 <= s < 2^-67.988.  */
static uint64_t
reduce_large (double *r, double x)
{
  uint64_t t = asuint64 (x);
  /* 1054 <= e <= 2046.  */
  int e = (t >> MANTISSA_WIDTH) & 0x7ff;
  uint64_t m = (UINT64_C(1) << MANTISSA_WIDTH) | get_mantissa (t);
  /* x = m * 2^(e-1075).  T[j] corresponds to T[j]/2^(64*j) thus T[j]*x
     corresponds to m*T[j]*2^(e-1075-64*j).  To get a non-zero fractional
     value, we need e-1075-64*j < 0, thus 64*j > e-1075 or 64*j >= e-1074.
     i = ceil((e-1074)/64), 0 <= i <= 16.  */
  int i = (e - 1011) / 64;
  int f = (e - 1011) & 0x3f;
  /* The number of fractional bits from m*T[i] is 64-f, thus T[i] should be
     shifted by f bits to get 64 fractional bits.  */
  uint64_t v0, v1;
  if (f == 0)
    {
      v0 = T[i];
      v1 = T[i + 1];
    }
  else
    {
      v0 = (T[i] << f) | (T[i + 1] >> (64 - f));
      v1 = (T[i + 1] << f) | (T[i + 2] >> (64 - f));
    }
  /* Computing directly u with 128-bit arithmetic from T[i], T[i+1] and
     T[i+2] is slower (128-bit arithmetic is emulated).  */
  u128 u = u128_mul (u128_from_u64 (m), u128_from_hl (v0, v1));
  /* Round r to nearest, where 0x810000000000000 = 2^59 + 2^52.  */
  u = u128_add (u, u128_from_hl (UINT64_C(1) << 48,
				 UINT64_C(0x810000000000000)));
  /* The next 53 bits of u after the first 15.  */
  double h = u128_low (u128_rshift (u128_lshift (u, 15), 75));
  *r = h * 0x1p-68 - 0x1p-16;
  /* Since 15 bits are returned in i and 53 in h, the accuracy is at most
     2^-68.  */
  return u128_high (u) >> 49;
}

static const double pih = -0x1.921fb54442d18p-13;
static const double pil = -0x1.1a62633145c07p-67;

#endif
