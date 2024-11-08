/* Internal 128 bit int support.
   Copyright (C) 2024 Free Software Foundation, Inc.
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

#ifndef _MATH_INT128_H
#define _MATH_INT128_H

/* Limited support for internal 128 bit integer.  Currently, only unsigned
   types is provided and if compiler supports it nativeally, use it instead.
   The provided wrapper are:
 
   - u128_t:         the 128 bit unsigned type.
   - u128_high:      return the high part of the number.
   - u128_low:       return the low part of the number.
   - u128_from_u64:  return a 128 bit number from a 64 bit one.
   - u128_mul:       multiple two numbers.
   - u128_add:       add two numbers.
   - u128_lshift:    left shift a number.
   - u128_rshift:    right shift a number.
 */

#ifdef __SIZEOF_INT128__
typedef unsigned __int128 u128;
# define u128_high(__x)         (uint64_t)((__x) >> 64)
# define u128_low(__x)          (uint64_t)(__x)
# define u128_from_u64(__x)     (u128)(__x)
# define u128_mul(__x, __y)     (__x) * (__y)
# define u128_add(__x, __y)     (__x) + (__y)
# define u128_lshift(__x, __y)  (__x) << (__y)
# define u128_rshift(__x, __y)  (__x) >> (__y)
#else
typedef struct
{
  uint64_t low;
  uint64_t high;
} u128;

# define u128_high(__x)         (__x).high
# define u128_low(__x)          (__x).low

# define u128_from_u64(__x)     (u128){.low = (__x), .high = 0}

# define TOPBIT                 (UINT64_C(1) << 63)
# define MASK32                 (UINT64_C(0xffffffff))

static u128 u128_add (u128 x, u128 y)
{
  uint64_t lower = (x.low & ~TOPBIT) + (y.low & ~TOPBIT);
  bool carry = (lower >> 63) + (x.low >> 63) + (y.low >> 63) > 1;
  return (u128) { .high = x.high + y.high + carry, .low = x.low + y.low };
}

static u128 u128_lshift (u128 x, int n)
{
  if (n >= 128)
    return (u128) { 0, 0 };
  else if (n == 0)
    return x;
  else
   {
     if (n >= 64)
       return (u128) { .high = x.low << (n - 64), .low = 0};
     else
       return (u128) { .high = (x.high << n) | (x.low >> (64 - n)),
	   	       .low = x.low << n };
   }
}

static u128 u128_rshift (u128 x, int n)
{
  if (n >= 128)
    return (u128) { 0, 0 };
  else if (n == 0)
    return x;
  else
   {
     if (n >= 64)
       return (u128) { .high = 0, .low = x.high >> (n - 64) };
     else
       return (u128) { .high = x.high >> n,
	   	       .low = (x.high << (64 - n)) | (x.low >> n) };
   }
}

static u128 u128_mul (u128 x, u128 y)
{
  if (x.high == 0 && y.high == 0)
    {
      uint64_t x0 = x.low & MASK32;
      uint64_t x1 = x.low >> 32;
      uint64_t y0 = y.low & MASK32;
      uint64_t y1 = y.low >> 32;
      u128 x0y0 = { .high = 0, .low = x0 * y0 };
      u128 x0y1 = { .high = 0, .low = x0 * y1 };
      u128 x1y0 = { .high = 0, .low = x1 * y0 };
      u128 x1y1 = { .high = 0, .low = x1 * y1 };
      /* x0y0 + ((x0y1 + x1y0) << 32) + (x1y1 << 64)  */
      return u128_add (u128_add (x0y0, u128_lshift (u128_add (x0y1,
							      x1y0), 32)),
		       u128_lshift (x1y1, 64));
    }
  else
    {
      uint64_t x0 = x.low & MASK32;
      uint64_t x1 = x.low >> 32;
      uint64_t x2 = x.high & MASK32;
      uint64_t x3 = x.high >> 32;
      uint64_t y0 = y.low & MASK32;
      uint64_t y1 = y.low >> 32;
      uint64_t y2 = y.high & MASK32;
      uint64_t y3 = y.high >> 32;
      u128 x0y0 = { .high = 0, .low = x0 * y0 };
      u128 x0y1 = { .high = 0, .low = x0 * y1 };
      u128 x0y2 = { .high = 0, .low = x0 * y2 };
      u128 x0y3 = { .high = 0, .low = x0 * y3 };
      u128 x1y0 = { .high = 0, .low = x1 * y0 };
      u128 x1y1 = { .high = 0, .low = x1 * y1 };
      u128 x1y2 = { .high = 0, .low = x1 * y2 };
      u128 x2y0 = { .high = 0, .low = x2 * y0 };
      u128 x2y1 = { .high = 0, .low = x2 * y1 };
      u128 x3y0 = { .high = 0, .low = x3 * y0 };
      /* x0y0 + ((x0y1 + x1y0) << 32) + ((x0y2 + x1y1 + x2y0) << 64) +
          ((x0y3 + x1y2 + x2y1 + x3y0) << 96)  */
      u128 r0 = u128_add (x0y0,
			  u128_lshift (u128_add (x0y1, x1y0), 32));
      u128 r1 = u128_add (u128_lshift (u128_add (u128_add (x0y2, x1y1), x2y0),
				       64),
			  u128_lshift (u128_add (u128_add (x0y3, x1y2),
						 u128_add (x2y1, x3y0)), 96));
      return u128_add (r0, r1);
   }
}
#endif /* __SIZEOF_INT128__ */

#endif
