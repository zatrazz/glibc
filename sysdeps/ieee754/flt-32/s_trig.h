/* Common definitions for cosf, sinf, and tanf.  */

#ifndef _S_TRIG_H
#define _S_TRIG_H

#include <stdint.h>
#include <math_uint128.h>

/* argument reduction
   for |x| >= 2^28, return r such that 2/pi*x = q + r  */
static double __attribute__ ((noinline))
rbig (uint32_t u, int *q, const int e_adj)
{
  static const uint64_t ipi[] =
    {
      0xfe5163abdebbc562, 0xdb6295993c439041,
      0xfc2757d1f534ddc0, 0xa2f9836e4e441529
    };
  int e = (u >> 23) & 0xff, i;
  uint64_t m = (u & (~0u >> 9)) | 1 << 23;
  u128 p0 = u128_mul (u128_from_u64 (m), u128_from_u64 (ipi[0]));
  u128 p1 = u128_mul (u128_from_u64 (m), u128_from_u64 (ipi[1]));
  p1 = u128_add (p1, u128_rshift (p0, 64));
  u128 p2 = u128_mul (u128_from_u64 (m), u128_from_u64 (ipi[2]));
  p2 = u128_add (p2, u128_rshift (p1, 64));
  u128 p3 = u128_mul (u128_from_u64 (m), u128_from_u64 (ipi[3]));
  p3 = u128_add (p3, u128_rshift (p2, 64));
  uint64_t p3h = u128_high (p3);
  uint64_t p3l = u128_low (p3);
  uint64_t p2l = u128_low (p2);
  uint64_t p1l = u128_low (p1);
  int64_t a;
  int k = e - e_adj, s = k - 23;
  /* in ctanf(), rbig() is called in the case 127+28 <= e < 0xff
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


#endif
