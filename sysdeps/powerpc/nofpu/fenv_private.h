/* Optimized inline fenv.h functions for libm.  PowerPC soft-float version.
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

#ifndef POWERPC_NOFPU_FENV_PRIVATE_H
#define POWERPC_NOFPU_FENV_PRIVATE_H 1

/* The floating-point environment is kept in the __sim_* thread
   variables, which the compiler does not know the soft-fp arithmetic
   routines update.  Inlining the accesses into libm would let it reuse
   the values stored before an operation, so keep calling the out-of-line
   functions, which act as a barrier.  */
#define FENV_PRIVATE_OUT_OF_LINE 1

#include_next <fenv_private.h>

#endif /* fenv_private.h */
