/* Common routines for sinf/cosf/tanf/sincosf.

Copyright (c) 2022-2026 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary32/tan/tanf.c, revision 59d21d7).

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

#include <math_uint128.h>
#include <s_sincosf_common.h>
#include <s_sincosf_data.h>

static double
rbig (uint32_t u, int *q, int e_bias)
{
  int e = (u >> 23) & 0xff, i;
  uint64_t m = (u & (~0u >> 9)) | 1 << 23;
  u128 p0 = u128_mul (u128_from_u64 (m), u128_from_u64 (IPI[0]));
  u128 p1 = u128_mul (u128_from_u64 (m), u128_from_u64 (IPI[1]));
  p1 = u128_add (p1, u128_rshift (p0, 64));
  u128 p2 = u128_mul (u128_from_u64 (m), u128_from_u64 (IPI[2]));
  p2 = u128_add (p2, u128_rshift (p1, 64));
  u128 p3 = u128_mul (u128_from_u64 (m), u128_from_u64 (IPI[3]));
  p3 = u128_add (p3, u128_rshift (p2, 64));
  uint64_t p3h = u128_high (p3), p3l = u128_low (p3), p2l = u128_low (p2),
	   p1l = u128_low (p1);
  int64_t a;
  int k = e - e_bias, s = k - 23;
  /* in cr_sinf(), rbig() is called in the case 127+28 <= e < 0xff
     thus 155 <= e <= 254, which yields 28 <= k <= 127 and 5 <= s <= 104 */
  if (s < 64)
    {
      i = p3h << s | p3l >> (64 - s);
      a = p3l << s | p2l >> (64 - s);
    }
  else if (s == 64)
    {
      i = p3l;
      a = p2l;
    }
  else
    { /* s > 64 */
      i = p3l << (s - 64) | p2l >> (128 - s);
      a = p2l << (s - 64) | p1l >> (128 - s);
    }
  int sgn = u;
  sgn >>= 31;
  int64_t sm = a >> 63;
  i -= sm;
  double z = (a ^ sgn) * 0x1p-64;
  i = (i ^ sgn) - sgn;
  *q = i;
  return z;
}

double
__sincosf_rbig (uint32_t u, int *q)
{
  /* in cr_sinf(), rbig() is called in the case 127+28 <= e < 0xff
     thus 155 <= e <= 254, which yields 28 <= k <= 127 and 5 <= s <= 104 */
  return rbig (u, q, 124);
}

/* argument reduction
   same as rltl, but for |x| >= 2^28  */
double
__tanf_rbig (uint32_t u, int *q)
{
  /* in ctanf(), rbig() is called in the case 127+28 <= e < 0xff
     thus 155 <= e <= 254, which yields 28 <= k <= 127 and 5 <= s <= 104 */
  return rbig (u, q, 127);
}
