/* Barrier for hiding the address of constant data from the compiler.
   AArch64 version.
   Copyright (C) 2026 Free Software Foundation, Inc.
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

#ifndef AARCH64_MATH_PTR_BARRIER_H
#define AARCH64_MATH_PTR_BARRIER_H 1

/* Return PTR but hide its value from the compiler so accesses through it
   cannot be optimized based on the contents.  Without it, GCC might folds
   each element of a static constant table into a separate literal.  */
#define ptr_barrier(ptr)						\
  ({									\
    __typeof ((ptr) + 0) __ptr = (ptr);					\
    __asm ("" : "+r" (__ptr));						\
    __ptr;								\
  })

#endif
