/* Zero byte detection; indexes.  Generic C version.
   Copyright (C) 2023-2025 Free Software Foundation, Inc.
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
   <http://www.gnu.org/licenses/>.  */

#ifndef _STRING_FZI_H
#define _STRING_FZI_H 1

#include <limits.h>
#include <endian.h>
#include <string-fza.h>
#include <math-use-builtins-bitops.h>

static __always_inline int
ctz (find_t c)
{
#if USE_CTZ_BUILTIN
  if (sizeof (find_t) == sizeof (unsigned long))
    return __builtin_ctzl (c);
  else
    return __builtin_ctzll (c);
#else
  if (sizeof (find_t) < 8)
    {
      static const char debruijn32[]
	  = { 0,  1,  23, 2,  29, 24, 19, 3,  30, 27, 25, 11, 20, 8, 4,	 13,
	      31, 22, 28, 18, 26, 10, 7,  12, 21, 17, 9,  6,  16, 5, 15, 14 };
      return debruijn32[(c & -c) * 0x076be629 >> 27];
    }
  else
    {
      static const char debruijn64[]
	  = { 0,  1,  2,  53, 3,  7,  54, 27, 4,  38, 41, 8,  34, 55, 48, 28,
	      62, 5,  39, 46, 44, 42, 22, 9,  24, 35, 59, 56, 49, 18, 29, 11,
	      63, 52, 6,  26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
	      51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12 };
      return debruijn64[(c & -c) * 0x022fdd63cc95386dull >> 58];
    }
#endif
}

static __always_inline int
clz (find_t c)
{
#if USE_CTZ_BUILTIN
  if (sizeof (find_t) == sizeof (unsigned long))
    return __builtin_clzl (c);
  else
    return __builtin_clzll (c);
#else
  if (sizeof (find_t) < 8)
    {
      c >>= 1;
      c |= c >> 1;
      c |= c >> 2;
      c |= c >> 4;
      c |= c >> 8;
      c |= c >> 16;
      c++;
      return 31 - ctz (c);
    }
  else
    {
      unsigned long long int c0 = c;
      find_t y;
      find_t r;
      if (c0 >> 32)
	y = c0 >> 32, r = 0;
      else
	y = c0, r = 32;
      if (y >> 16)
	y >>= 16;
      else
	r |= 16;
      if (y >> 8)
	y >>= 8;
      else
	r |= 8;
      if (y >> 4)
	y >>= 4;
      else
	r |= 4;
      if (y >> 2)
	y >>= 2;
      else
	r |= 2;
      return r | !(y >> 1);
    }
#endif
}

/* A subroutine for the index_zero functions.  Given a test word C, return
   the (memory order) index of the first byte (in memory order) that is
   non-zero.  */
static __always_inline unsigned int
index_first (find_t c)
{
  int r;
  if (__BYTE_ORDER == __LITTLE_ENDIAN)
    r = ctz (c);
  else
    r = clz (c);
  return r / CHAR_BIT;
}

/* Similarly, but return the (memory order) index of the last byte that is
   non-zero.  */
static __always_inline unsigned int
index_last (find_t c)
{
  int r;
  if (__BYTE_ORDER == __LITTLE_ENDIAN)
    r = clz (c);
  else
    r = ctz (c);
  return sizeof (find_t) - 1 - (r / CHAR_BIT);
}

#endif /* STRING_FZI_H */
