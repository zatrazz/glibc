/* Tunables set by glibc.rtld.hardened.  MIPS Linux version.
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

/* glibc is built with an executable stack for the FPU emulation of
   kernels older than 4.8.  */
#ifdef MIPS_FORCE_EXECSTACK
# define DL_HARDENED_SKIP_EXECSTACK 1
#endif

#include_next <dl-hardened.h>
