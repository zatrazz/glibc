/* Pointer obfuscation implenentation.  Generic (no-op) version.
   Copyright (C) 2022 Free Software Foundation, Inc.
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

#ifndef POINTER_GUARD_H
#define POINTER_GUARD_H

/* Assembler code depends on PTR_MANGLE not being defined for
   optimization purposes.  */
#ifndef __ASSEMBLER__
# define PTR_MANGLE(x) (void) (x)
# define PTR_DEMANGLE(x) (void) (x)
#endif

/* Define whether the ABI store the pointer guard canary in the in thread
   local area or as global variable.  It is used to export the
   __pointer_chk_guard by the loader.

   Define either POINTER_GUARD_BY_GLOBAL (global variable) or
   POINTER_GUARD_BY_TCB (thread local area).   */

#define POINTER_GUARD_BY_GLOBAL 1

#endif /* POINTER_GUARD_H */
