/* Correctly-rounded binary64 helper functions for int128 function.

Copyright (c) 2022-2025 Alexei Sibidanov.

This file is part of the CORE-MATH project
(https://core-math.gitlabpages.inria.fr/).

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

#ifndef _U128_U_H
#define _U128_U_H

#include <math_uint128.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
typedef union {u128 a; struct {uint64_t bl, bh;};} u128_u;
#else
typedef union {u128 a; struct {uint64_t bh, bl;};} u128_u;
#endif

inline static void shl(u128_u *a, int n)
{
  /* (*a).a <<= n */
  (*a).a = u128_lshift ((*a).a, n);
}

inline static void shr(u128_u *a, int n)
{
  /* (*a).a >>= n */
  (*a).a = u128_rshift ((*a).a, n);
}

inline static uint64_t muuh(uint64_t a, uint64_t b)
{
  /* return (a*(u128)b)>>64; */
  return u128_high (u128_mul (u128_from_i64 (a), u128_from_i64 (b)));
}

inline static int64_t mh(int64_t a, int64_t b)
{
  /*return (int64_t)((a*(i128)b)>>64); */
  return i128_high (i128_mul (i128_from_i64 (a), i128_from_i64 (b)));
}

inline static i128 imul(int64_t a, int64_t b)
{
  /*return a*(i128)b; */
  return i128_mul (i128_from_i64 (a), i128_from_i64 (b));
}

inline static u128 mUU(u128 a, u128 b)
{
  u128_u x = {.a = a}, y = {.a = b};
  /*
  u128 o = x.bh*(u128)y.bh;
  o += (uint64_t)(x.bl*(u128)y.bh>>64);
  o += (uint64_t)(x.bh*(u128)y.bl>>64);
  */
  u128 o = u128_mul (u128_from_u64 (x.bh), u128_from_u64 (y.bh));
  o = u128_add (o,
		u128_from_u64 (u128_high (u128_mul (u128_from_u64 (x.bl),
						    u128_from_u64 (y.bh)))));
  o = u128_add (o,
		u128_from_u64 (u128_high (u128_mul (u128_from_u64 (x.bh),
						    u128_from_u64 (y.bl)))));
  return o;
}

inline static u128 muU (uint64_t a, u128 b)
{
  u128_u y = {.a = b};
  /*
  u128 o = a*(u128)y.bh;
  o += a*(u128)y.bl>>64;
  */
  u128 o = u128_mul (u128_from_u64 (a), u128_from_u64 (y.bh));
  o = u128_add (o,
		u128_rshift (u128_mul (u128_from_u64 (a),
				       u128_from_u64 (y.bl)), 64));
  return o;
}

inline static u128 sqrU (u128 a)
{
  u128_u x = {.a = a};
  /*
  u128 os = x.bl*(u128)x.bh>>63;
  u128 o = x.bh*(u128)x.bh;
  return o + os;
  */
  u128 os = u128_rshift (u128_mul (u128_from_u64 (x.bl),
				   u128_from_u64 (x.bh)), 63);
  u128 o = u128_mul (u128_from_u64 (x.bh), u128_from_u64 (x.bh));
  return u128_add (o, os);
}

static __attribute_maybe_unused__ u128 pasin(u128 x)
{
  uint64_t xh = u128_high (x);
  static const uint64_t b[] = { 0x5ba2e8ba2e8ad9b7, 0x0004713b13b29079,
				0x000000393331e196, 0x0000000002f5c315 };
  static const u128_u ch[] = {
    { .bl = 0xaaaaaaaaaaaaaaa5, .bh = 0x0002aaaaaaaaaaaa }, // *+1
    { .bl = 0x3333333333333484, .bh = 0x0000001333333333 }, // *+1
    { .bl = 0xb6db6db6db6da950, .bh = 0x0000000000b6db6d }, // *+1
    { .bl = 0x1c71c71c71c76217, .bh = 0x00000000000007c7 }, // *+1
  };
  u128_u t = ch[3];
  t.bl
     += muuh (xh, b[0] + muuh (xh, b[1] + muuh (xh, b[2] + muuh (xh, b[3]))));
  /* mUU (x, ch[0].a + mUU (x, ch[1].a + mUU (x, ch[2].a + mUU (x, t.a)))); */
  return mUU(x, u128_add (ch[0].a,
			  mUU(x,
			      u128_add (ch[1].a,
					mUU(x,
					   u128_add (ch[2].a,
						     mUU(x, t.a)))))));
}

#endif
