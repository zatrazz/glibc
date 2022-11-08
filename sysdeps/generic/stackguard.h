/* Stack guard internal definitions.  Generic version.
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

#ifndef _STACK_GUARD_H
#define _STACK_GUARD_H

/* Define whether the ABI store the stack guard canary in the in thread local
   area or as global variable.  It is used to export the __stack_chk_guard
   byt the loader.

   Define either STACK_GUARD_BY_GLOBAL (global variable) or
   STACK_GUARD_BY_TCB (thread local area).   */

#define STACK_GUARD_BY_GLOBAL 1

#endif /* _STACK_GUARD_H  */
