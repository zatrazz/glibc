/* Internal 128 bit int support.
   Copyright (C) 2024-2025 Free Software Foundation, Inc.
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

#include <stdint.h>
#include <stdbool.h>
#include <endian.h>

/* Limited support for internal 128 bit integer, used on some math
   implementations.  It uses compiler builtin type if supported, otherwise
   it is emulated.  Only unsigned and some operations are currently supported:

   - u128_t:         the 128 bit unsigned type.
   - u128_high:      return the high part of the number.
   - u128_low:       return the low part of the number.
   - u128_from_u64:  create a 128 bit number from a 64 bit one.
   - u128_mul:       multiply two 128 bit numbers.
   - u128_add:       add two 128 bit numbers.
   - u128_lshift:    left shift a number.
   - u128_rshift:    right shift a number.
 */

#if defined __BITINT_MAXWIDTH__ && __BITINT_MAXWIDTH__ >= 128
typedef unsigned _BitInt(128) u128;
typedef          _BitInt(128) i128;
# define __MATH_INT128_BUILTIN_TYPE 1
#elif defined __SIZEOF_INT128__
typedef unsigned __int128 u128;
typedef          __int128 i128;
# define __MATH_INT128_BUILTIN_TYPE 1
#else
# define __MATH_INT128_BUILTIN_TYPE 0
#endif


#if __MATH_INT128_BUILTIN_TYPE
# define u128_high(__x)         (uint64_t)((__x) >> 64)
# define u128_low(__x)          (uint64_t)(__x)
# define u128_from_u64(__x)     (u128)(__x)
# define u128_from_u64_hl(__hi, __lo)  ((u128)(__hi)<<64|__lo)
# define u128_from_i64(__x)     (u128)(__x)
# define u128_from_i128(__x)    (u128)(__x)
# define u128_mul(__x, __y)     (__x) * (__y)
# define u128_add(__x, __y)     (__x) + (__y)
# define u128_sub(__x, __y)     (__x) - (__y)
# define u128_lshift(__x, __y)  (__x) << (__y)
# define u128_rshift(__x, __y)  (__x) >> (__y)
# define u128_or(__x, __y)      (__x) | (__y)
# define u128_and(__x, __y)      (__x) & (__y)
# define u128_gt(__x, __y)      ((__x) > (__y))
# define u128_lt(__x, __y)      ((__x) < (__y))
# define u128_eq(__x, __y)      ((__x) == (__y))
# define u128_bitwise_and(__x, __y) ((__x) & (__y))
# define u128_bitwise_or(__x, __y) ((__x) | (__y))
# define u128_bitwise_xor(__x, __y) ((__x) ^ (__y))
# define u128_logical_not(__x)  (!(__x))

# define i128_high(__x)         (int64_t)((__x) >> 64)
# define i128_low(__x)          (int64_t)(__x)
# define i128_from_u64(__x)     (i128)(__x)
# define i128_from_i64(__x)     (i128)(__x)
# define i128_from_u128(__x)    (i128)(__x)
# define i128_mul(__x, __y)     (__x) * (__y)
# define i128_add(__x, __y)     (__x) + (__y)
# define i128_sub(__x, __y)     (__x) - (__y)
# define i128_lshift(__x, __y)  (__x) << (__y)
# define i128_rshift(__x, __y)  (__x) >> (__y)
# define i128_and(__x, __y)     (__x) & (__y)
#else
typedef struct
{
# if __BYTE_ORDER == __LITTLE_ENDIAN
  uint64_t low;
  uint64_t high;
#else
  uint64_t high;
  uint64_t low;
#endif
} u128;

typedef struct
{
# if __BYTE_ORDER == __LITTLE_ENDIAN
  uint64_t low;
  uint64_t high;
#else
  uint64_t high;
  uint64_t low;
#endif
} i128;

# define u128_high(__x)         (__x).high
# define u128_low(__x)          (__x).low
# define u128_from_u64(__x)     (u128){.low = (__x), .high = 0}
# define u128_from_u64_hl(__hi, __lo)  (u128){.low = (__lo), .high =(__hi)}
# define u128_from_i64(__x)     (u128){.low = (uint64_t)(__x), \
                                       .high = -(uint64_t)(__x < 0)}
# define u128_from_i128(__x)    (u128){.low = (__x).low, .high = (__x).high}
# define u128_or(__x, __y)      (u128){.low  = (__x).low  | (__y).low, \
                                       .high = (__x).high | (__y).high }
# define u128_and(__x, __y)      (u128){.low  = (__x).low  & (__y).low, \
                                        .high = (__x).high & (__y).high }
# define u128_bitwise_and(__x, __y)  (u128){.low = (__x).low & (__y).low, \
					    .high = (__x).high & (__y).high }
# define u128_bitwise_or(__x, __y)  (u128){.low = (__x).low | (__y).low, \
					    .high = (__x).high | (__y).high }
# define u128_bitwise_xor(__x, __y)  (u128){.low = (__x).low ^ (__y).low, \
					    .high = (__x).high ^ (__y).high }

# define i128_high(__x)         (int64_t)(__x).high
# define i128_low(__x)          (int64_t)(__x).low
# define i128_from_u64(__x)     (i128){.low = (__x), .high = 0}
# define i128_from_i64(__x)     (i128){.low = (uint64_t)(__x), \
                                       .high = -(uint64_t)(__x < 0)}
# define i128_from_u128(__x)    (i128){.low = (__x).low, .high = (__x).high }

# define i128_and(__x, __y)     (i128){.low  = (__x).low  & (__y).low, \
                                       .high = (__x).high & (__y).high }
# define i128_bitwise_not(__x)  (i128){.low = ~(__x).low, .high = (__x).high }

# define MASK32                 (UINT64_C(0xffffffff))
# define TOPBIT                 (UINT64_C(1) << 63)

static u128 u128_add (u128 x, u128 y)
{
  bool carry = x.low + y.low < x.low;
  return (u128) { .high = x.high + y.high + carry, .low = x.low + y.low };
}

static u128 u128_neg (u128 x)
{
  u128 xbitnot = (u128){.high = ~x.high, .low = ~x.low};
  return u128_add (xbitnot, u128_from_u64 (1));
}

static __attribute_maybe_unused__ u128 u128_sub (u128 x, u128 y)
{
  return u128_add (x, u128_neg (y));
}

static i128 i128_add (i128 x, i128 y)
{
  bool carry = x.low + y.low < x.low;
  return (i128) { .high = x.high + y.high + carry, .low = x.low + y.low };
}

static i128 i128_neg (i128 x)
{
  i128 xbitnot = (i128){.high = ~x.high, .low = ~x.low};
  return i128_add (xbitnot, i128_from_u64 (1));
}

static __attribute_maybe_unused__ i128 i128_sub (i128 x, i128 y)
{
  return i128_add (x, i128_neg (y));
}

static u128 u128_lshift (u128 x, unsigned int n)
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

static i128 i128_lshift (i128 x, unsigned int n)
{
  switch (n)
    {
    case 0:         return x;
    case 1 ... 63:  return (i128) { .high = (x.high << n) | (x.low >> (64 - n)),
				    .low = x.low << n };
    case 64 ...127: return (i128) { .high = x.low << (n - 64), .low = 0};
    default:        return (i128) { .high = 0, .low = 0 };
    }
}

static u128 __attribute_maybe_unused__ u128_rshift (u128 x, unsigned int n)
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

/* Right shift are implemented as arithmentic shifts by both gcc/clang.  */
static __attribute_maybe_unused__ i128 i128_rshift (i128 x, unsigned int n)
{
  switch (n)
    {
    case 0:
      return x;
    case 1 ... 63:
      {
        /* Arithmetic with sign propagation from high to low.  */
	int64_t shigh = (int64_t) (x.high);
	return (i128){ .high = (uint64_t) (shigh >> n),
		       .low = (x.high << (64 - n)) | (x.low >> n) };
      }
    case 64 ... 127:
      {
	int64_t shigh = (int64_t) (x.high);
	return (i128) { .high = (uint64_t)(shigh >> 63),
			.low = (uint64_t)(shigh >> (n - 64)) };
      }
    default:
      {
        /* Result is all sign bits.  */
	int64_t shigh = (int64_t) (x.high);
	uint64_t fill = (shigh < 0) ? UINT64_MAX : 0;
	return (i128){ .high = fill, .low = fill };
      }
    }
}

static __attribute_maybe_unused__ u128
u128_mul (u128 x, u128 y)
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
      u128 x1y1 = { .high = x1 * y1, .low = 0 };
      /* x0y0 + ((x0y1 + x1y0) << 32) + x1y1  */
      return u128_add (u128_add (x0y0,
				 u128_lshift (u128_add (x0y1, x1y0),
					      32)),
		       x1y1);
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
			  u128_lshift (u128_add (x0y1, x1y0),
				       32));
      u128 r1 = u128_add (u128_lshift (u128_add (u128_add (x0y2, x1y1), x2y0),
				       64),
			  u128_lshift (u128_add (u128_add (x0y3, x1y2),
						 u128_add (x2y1, x3y0)),
				       96));
      return u128_add (r0, r1);
   }
}

static inline int u128_lt (u128 x, u128 y)
{
  return x.high < y.high || (x.high == y.high && x.low < y.low);
}

static inline int u128_gt (u128 x, u128 y)
{
  return u128_lt (y, x);
}

static inline int u128_eq (u128 x, u128 y)
{
  return x.low == y.low && x.high == y.high;
}

static inline bool u128_logical_not (u128 x)
{
  return !x.low && !x.high;
}

static __attribute_maybe_unused__ i128 i128_mul (i128 x, i128 y)
{
  if (x.high == 0 && y.high == 0)
    {
      uint64_t x0 = x.low & MASK32;
      uint64_t x1 = x.low >> 32;
      uint64_t y0 = y.low & MASK32;
      uint64_t y1 = y.low >> 32;
      i128 x0y0 = i128_from_u64 (x0 * y0);
      i128 x0y1 = i128_from_u64 (x0 * y1);
      i128 x1y0 = i128_from_u64 (x1 * y0);
      i128 x1y1 = i128_from_u64 (x1 * y1);
      /* x0y0 + ((x0y1 + x1y0) << 32) + (x1y1 << 64)  */
      return i128_add (i128_add (x0y0,
				 i128_lshift (i128_add (x0y1, x1y0),
					      32)),
		       i128_lshift (x1y1, 64));
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
      i128 x0y0 = i128_from_u64 (x0 * y0);
      i128 x0y1 = i128_from_u64 (x0 * y1);
      i128 x0y2 = i128_from_u64 (x0 * y2);
      i128 x0y3 = i128_from_u64 (x0 * y3);
      i128 x1y0 = i128_from_u64 (x1 * y0);
      i128 x1y1 = i128_from_u64 (x1 * y1);
      i128 x1y2 = i128_from_u64 (x1 * y2);
      i128 x2y0 = i128_from_u64 (x2 * y0);
      i128 x2y1 = i128_from_u64 (x2 * y1);
      i128 x3y0 = i128_from_u64 (x3 * y0);
      /* x0y0 + ((x0y1 + x1y0) << 32) + ((x0y2 + x1y1 + x2y0) << 64) +
          ((x0y3 + x1y2 + x2y1 + x3y0) << 96)  */
      i128 r0 = i128_add (x0y0,
			  i128_lshift (i128_add (x0y1, x1y0),
				       32));
      i128 r1 = i128_add (i128_lshift (i128_add (i128_add (x0y2, x1y1), x2y0),
				       64),
			  i128_lshift (i128_add (i128_add (x0y3, x1y2),
						 i128_add (x2y1, x3y0)),
				       96));
      return i128_add (r0, r1);
   }
}
#endif /* __SIZEOF_INT128__ */

#endif
