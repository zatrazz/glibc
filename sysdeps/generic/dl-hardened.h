/* Tunables set by glibc.rtld.hardened.  Generic version.
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

#ifndef _DL_HARDENED_H
#define _DL_HARDENED_H

/* Define to 1 if glibc.rtld.hardened must not set glibc.rtld.wx_segments to 0,
   because the ABI places the procedure linkage table in a writable and
   executable segment, so that every dynamically linked program would be
   rejected.  */
#ifndef DL_HARDENED_SKIP_WX_SEGMENTS
# define DL_HARDENED_SKIP_WX_SEGMENTS 0
#endif

/* Define to 1 if glibc.rtld.hardened must not set glibc.rtld.execstack to 0,
   because glibc itself requires an executable stack.  */
#ifndef DL_HARDENED_SKIP_EXECSTACK
# define DL_HARDENED_SKIP_EXECSTACK 0
#endif

#endif
