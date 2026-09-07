/* Architecture-specific implementation of the <fenv.h> functions.
   MIPS soft-float version.
   Copyright (C) 2018-2026 Free Software Foundation, Inc.
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

#ifndef MIPS_NOFPU_FENV_IMPL_H
#define MIPS_NOFPU_FENV_IMPL_H 1

/* MIPS bits/fenv.h used to define exception macros for soft-float
   despite that not supporting exceptions.  Ensure use of the old
   FE_NOMASK_ENV value still produces errors (see bug 17088).  The
   generic stubs are parsed with the old values so that fenv_setenv and
   fenv_updateenv reject that environment, and the macros are restored
   afterwards so that the rest of the translation unit sees the
   <fenv.h> values.  */
#include <fenv.h>
#undef FE_ALL_EXCEPT
#define FE_ALL_EXCEPT 0x7c
#define FE_NOMASK_ENV ((const fenv_t *) -2)
#include_next <fenv-impl.h>
#undef FE_ALL_EXCEPT
#define FE_ALL_EXCEPT 0
#undef FE_NOMASK_ENV

#endif /* fenv-impl.h */
