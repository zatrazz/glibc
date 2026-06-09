/* Configure optimized libm functions.  RISC-V version.
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

#ifndef RISCV_MATH_PRIVATE_H
#define RISCV_MATH_PRIVATE_H 1

#include <stdint.h>
#include <math.h>

#if __riscv_xlen == 64
# define TOINT_INTRINSICS 1

static inline int32_t
converttoint (double x)
{
  int32_t r;
  __asm__ ("fcvt.w.d %0, %1, rne" : "=r" (r) : "f" (x));
  return r;
}

static inline double
roundtoint (double x)
{
# ifdef __riscv_zfa
  double r;
  __asm__ ("fround.d %0, %1, rne" : "=f" (r) : "f" (x));   /* float->float */
  return r;
# else
  /* |x| >= 2^52 is already integral (covers inf/NaN too) -> return as-is. */
  if (__builtin_fabs (x) < 0x1p52)
    {
      long i;
      double r;
      __asm__ ("fcvt.l.d %0, %1, rne" : "=r" (i) : "f" (x));
      __asm__ ("fcvt.d.l %0, %1"      : "=f" (r) : "r" (i));
      return r;
    }
  return x;
# endif
}
#endif

#include_next <math_private.h>

#endif
