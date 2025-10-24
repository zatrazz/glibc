/* Multiprecision generic functions.
   Copyright (C) 2025 Free Software Foundation, Inc.
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

#ifndef _MPN_H
#define _MPN_H

#include <gmp.h>

static __always_inline void
umul_ppmm_generic (mp_limb_t *w1, mp_limb_t *w0, mp_limb_t u, mp_limb_t v)
{
#ifdef __x86_64__
  asm ("mul{q} %3"
       : "=a" (*w0),
         "=d" (*w1)
       : "%0" (u),
         "rm" (v));
#else
  asm ("mul{l} %3"
       : "=a" (*w0),
         "=d" (*w1)
       : "%0" (u),
         "rm" (v));
#endif
}
#undef umul_ppmm
#define umul_ppmm(__w1, __w0, __u, __v) \
  umul_ppmm_generic (&__w1, &__w0, __u, __v)

static __always_inline void
udiv_qrnnd_x86 (mp_limb_t *q, mp_limb_t *r, mp_limb_t n1, mp_limb_t n0,
		mp_limb_t d)
{
#ifdef __x86_64__
  asm ("div{q} %4"
       : "=a" (*q),
         "=d" (*r)
       : "0" (n0),
	 "1" (n1),
	 "rm" (d));
#else
  asm ("div{l} %4"
       : "=a" (*q),
         "=d" (*r)
       : "0" (n0),
	 "1" (n1),
	 "rm" (d));
#endif
}
#undef UDIV_NEEDS_NORMALIZATION
#define UDIV_NEEDS_NORMALIZATION 0
#undef udiv_qrnnd
#define udiv_qrnnd(__q, __r, __n1, __n0, __d) \
  udiv_qrnnd_x86 (&__q, &__r, __n1, __n0, __d)

static __always_inline void
add_ssaaaa_x86 (mp_limb_t *sh, mp_limb_t *sl, mp_limb_t ah,
		mp_limb_t al, mp_limb_t bh, mp_limb_t bl)
{
#ifdef __x86_64__
  asm ("add{q} {%5,%1|%1,%5}\n\tadc{q} {%3,%0|%0,%3}"
       : "=r" (*sh),
         "=&r" (*sl)
       : "%0" (ah),
         "rme" (bh),
	 "%1" (al),
	 "rme" (bl));
#else
  asm ("add{l} {%5,%1|%1,%5}\n\tadc{l} {%3,%0|%0,%3}"
       : "=r" (*sh),
         "=&r" (*sl)
       : "%0" (ah),
          "g" (bh),
	  "%1" (al),
	  "g" (bl));
#endif
}
#undef add_ssaaaa
#define add_ssaaaa(__sh, __sl, __ah, __al, __bh, __bl) \
  add_ssaaaa_x86 (&__sh, &__sl, __ah, __al, __bh, __bl)

static __always_inline void
sub_ddmmss_x86 (mp_limb_t *sh, mp_limb_t *sl, mp_limb_t ah,
		mp_limb_t al, mp_limb_t bh, mp_limb_t bl)
{
#ifdef __x86_64__
  asm ("sub{q} {%5,%1|%1,%5}\n\tsbb{q} {%3,%0|%0,%3}"
       : "=r" (*sh),
         "=&r" (*sl)
       : "0" (ah),
         "rme" (bh),
	 "1" (al),
	 "rme" (bl));
#else
  asm ("sub{l} {%5,%1|%1,%5}\n\tsbb{l} {%3,%0|%0,%3}"
       : "=r" (*sh),
         "=&r" (*sl)
       : "0" (ah),
         "g" (bh),
	 "1" (al),
	 "g" (bl));
#endif
}
#undef sub_ddmmss
#define sub_ddmmss(__sh, __sl, __ah, __al, __bh, __bl) \
  sub_ddmmss_x86 (&__sh, &__sl, __ah, __al, __bh, __bl)

#endif
