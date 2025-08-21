/* Correctly-rounded natural exponential function for binary32 value.

Copyright (c) 2023-2025 Alexei Sibidanov.

TODO

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

#ifdef __expf
# undef libm_hidden_proto
# define libm_hidden_proto(ignored)
#endif

#include <math.h>
#include <stdint.h>
#include <libm-alias-finite.h>
#include <libm-alias-float.h>
#include "math_config.h"

float
__expf (float x)
{
  static const double c[] =
    {
      0x1.62e42fefa39efp-1, 0x1.ebfbdff82c58fp-3,  0x1.c6b08d702e0edp-5,
      0x1.3b2ab6fb92e5ep-7, 0x1.5d886e6d54203p-10, 0x1.430976b8ce6efp-13
    };
  static const double b[] =
    {
      1, 0x1.62e42fef4c4e7p-1, 0x1.ebfd1b232f475p-3, 0x1.c6b19384ecd93p-5
    };
  static const uint64_t tb[] =
    {
      0x3ff0000000000000, 0x3ff02c9a3e778061, 0x3ff059b0d3158574,
      0x3ff0874518759bc8, 0x3ff0b5586cf9890f, 0x3ff0e3ec32d3d1a2,
      0x3ff11301d0125b51, 0x3ff1429aaea92de0, 0x3ff172b83c7d517b,
      0x3ff1a35beb6fcb75, 0x3ff1d4873168b9aa, 0x3ff2063b88628cd6,
      0x3ff2387a6e756238, 0x3ff26b4565e27cdd, 0x3ff29e9df51fdee1,
      0x3ff2d285a6e4030b, 0x3ff306fe0a31b715, 0x3ff33c08b26416ff,
      0x3ff371a7373aa9cb, 0x3ff3a7db34e59ff7, 0x3ff3dea64c123422,
      0x3ff4160a21f72e2a, 0x3ff44e086061892d, 0x3ff486a2b5c13cd0,
      0x3ff4bfdad5362a27, 0x3ff4f9b2769d2ca7, 0x3ff5342b569d4f82,
      0x3ff56f4736b527da, 0x3ff5ab07dd485429, 0x3ff5e76f15ad2148,
      0x3ff6247eb03a5585, 0x3ff6623882552225, 0x3ff6a09e667f3bcd,
      0x3ff6dfb23c651a2f, 0x3ff71f75e8ec5f74, 0x3ff75feb564267c9,
      0x3ff7a11473eb0187, 0x3ff7e2f336cf4e62, 0x3ff82589994cce13,
      0x3ff868d99b4492ed, 0x3ff8ace5422aa0db, 0x3ff8f1ae99157736,
      0x3ff93737b0cdc5e5, 0x3ff97d829fde4e50, 0x3ff9c49182a3f090,
      0x3ffa0c667b5de565, 0x3ffa5503b23e255d, 0x3ffa9e6b5579fdbf,
      0x3ffae89f995ad3ad, 0x3ffb33a2b84f15fb, 0x3ffb7f76f2fb5e47,
      0x3ffbcc1e904bc1d2, 0x3ffc199bdd85529c, 0x3ffc67f12e57d14b,
      0x3ffcb720dcef9069, 0x3ffd072d4a07897c, 0x3ffd5818dcfba487,
      0x3ffda9e603db3285, 0x3ffdfc97337b9b5f, 0x3ffe502ee78b3ff6,
      0x3ffea4afa2a490da, 0x3ffefa1bee615a27, 0x3fff50765b6e4540,
      0x3fffa7c1819e90d8
    };
  const double iln2 = 0x1.71547652b82fep+0, big = 0x1.8p46;
  uint32_t t = asuint (x);
  double z = x, a = iln2 * z;
  uint64_t u = asuint64 (a + big);
  uint32_t ux = t << 1;
  if (__glibc_unlikely (ux > 0x8562e42eu || ux < 0x6f93813eu))
    {
      if (__glibc_likely (ux < 0x6f93813eu))
	return 1.0 + z * (1 + z * 0.5);
      if (ux >= 0xffu << 24)
	{ // x is inf or nan
	  if (ux > 0xffu << 24)
	    return x + x; // x = nan
	  static const float ir[] = { INFINITY, 0.0f };
	  return ir[t >> 31]; // x = +-inf
	}
      if (t > 0xc2ce8ec0u)
	{
	  double y
	      = 0x1p-149 + (z + 0x1.9d1d9fccf477p+6) * 0x1.71547652b82edp-150;
	  y = fmax_finite (y, 0x1p-151);
	  float r = y;
	  if (r == 0.0f)
	    return __math_uflowf (0);
	  return r;
	}
      if (!(t >> 31) && t > 0x42b17217u)
	{
	  float r = 0x1p127f * 0x1p127f;
	  if (r > 0x1.fffffep127f)
	    __math_oflowf (0);
	  return r;
	}
    }
  double ia = big - asdouble (u), h = a + ia;
  double sv = asdouble (tb[u & 0x3f] + ((u >> 6) << 52));
  double h2 = h * h, r = ((b[0] + h * b[1]) + h2 * (b[2] + h * (b[3]))) * sv;
  float ub = r, lb = r - r * 1.45e-10;
  if (__glibc_unlikely (ub != lb))
    {
      const double iln2h = 0x1.7154765p+0, iln2l = 0x1.5c17f0bbbe88p-31;
      h = (iln2h * z + ia) + iln2l * z;
      double s = sv;
      h2 = h * h;
      double w = s * h;
      r = s + w	* ((c[0] + h * c[1])
		   + h2 * ((c[2] + h * c[3]) + h2 * (c[4] + h * c[5])));
      ub = r;
    }
  return ub;
}

#ifndef __expf
hidden_def (__expf)
strong_alias (__expf, __ieee754_expf)
libm_alias_finite (__ieee754_expf, __expf)
versioned_symbol (libm, __expf, expf, GLIBC_2_27);
libm_alias_float_other (__exp, exp)
#endif
