/* Internal 128 bit int support.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.
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

#include <stdbool.h>
#include <intprops.h>

/* Limited support for internal 128 bit integer, used on some math
   implementations.  It uses compiler builtin type if supported, otherwise
   it is emulated.  Only unsigned and some operations are currently supported:

   - u128_t:         the 128 bit unsigned type.
   - u128_high:      return the high part of the number.
   - u128_low:       return the low part of the number.
   - u128_from_u64:  create a 128 bit number from a 64 bit one.
   - u128_from_hl:   create a 128 bit number from two 64 bit numbers.
   - u128_mul:       multiply two 128 bit numbers.
   - u128_add:       add two 128 bit numbers.
   - u128_sub:       subtract two 128 bit numbers.
   - u128_neg:       negate a 128 bit number.
   - u128_lshift:    left shift a number.
   - u128_rshift:    right shift a number.
 */

#if defined __BITINT_MAXWIDTH__ && __BITINT_MAXWIDTH__ >= 128
typedef unsigned _BitInt(128) u128;
# define __MATH_INT128_BUILTIN_TYPE 1
#elif defined __SIZEOF_INT128__
typedef unsigned __int128 u128;
# define __MATH_INT128_BUILTIN_TYPE 1
#else
# define __MATH_INT128_BUILTIN_TYPE 0
#endif

#if __MATH_INT128_BUILTIN_TYPE
# define u128_high(__x)         (uint64_t)((__x) >> 64)
# define u128_low(__x)          (uint64_t)(__x)
# define u128_from_u64(__x)     (u128)(__x)
# define u128_from_hl(__h, __l) (((u128)(__h) << 64) | (__l))
# define u128_mul(__x, __y)     (__x) * (__y)
# define u128_add(__x, __y)     (__x) + (__y)
# define u128_sub(__x, __y)     (__x) - (__y)
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
# define u128_from_hl(__h, __l) (u128){.low = (__l), .high = (__h)}

# define MASK32                 (UINT64_C(0xffffffff))

static inline u128 u128_add (u128 x, u128 y)
{
  bool carry = INT_ADD_WRAPV (x.low, y.low, &x.low);
  return (u128) { .high = x.high + y.high + carry, .low = x.low };
}

static inline u128 u128_sub (u128 x, u128 y)
{
  bool borrow = INT_SUBTRACT_WRAPV (x.low, y.low, &x.low);
  return (u128) { .high = x.high - y.high - borrow, .low = x.low };
}

static inline u128 u128_neg (u128 x)
{
  return u128_sub (u128_from_u64 (0), x);
}

static inline u128 u128_lshift (u128 x, unsigned int n)
{
  switch (n)
    {
    case 0:         return x;
    case 1 ... 63:  return (u128) { .high = (x.high << n) | (x.low >> (64 - n)),
				    .low = x.low << n };
    case 64 ...127: return (u128) { .high = x.low << (n - 64), .low = 0};
    default:        return (u128) { .high = 0, .low = 0 };
    }
}

static inline u128 u128_rshift (u128 x, unsigned int n)
{
  switch (n)
    {
    case 0:         return x;
    case 1 ... 63:  return (u128) { .high = x.high >> n,
				    .low = (x.high << (64 - n)) | (x.low >> n) };
    case 64 ...127: return (u128) { .high = 0, .low = x.high >> (n - 64) };
    default:        return (u128) { .high = 0, .low = 0 };
    }
}

static inline u128 u128_mul (u128 x, u128 y)
{
  /* Compute the low 128 bits of the product: a full 64x64->128
     multiplication of the low words done in 64 bit arithmetic, with the
     cross products folded into the high word (their upper halves would
     fall beyond bit 127).  */
  uint64_t x0 = x.low & MASK32;
  uint64_t x1 = x.low >> 32;
  uint64_t y0 = y.low & MASK32;
  uint64_t y1 = y.low >> 32;
  uint64_t x0y0 = x0 * y0;
  uint64_t x1y0 = x1 * y0;
  uint64_t x0y1 = x0 * y1;
  uint64_t x1y1 = x1 * y1;
  uint64_t cross = (x0y0 >> 32) + (x1y0 & MASK32) + (x0y1 & MASK32);
  uint64_t high = x1y1 + (x1y0 >> 32) + (x0y1 >> 32) + (cross >> 32);
  return (u128) { .high = high + x.low * y.high + x.high * y.low,
		  .low = x.low * y.low };
}
#endif /* __SIZEOF_INT128__ */

#endif
