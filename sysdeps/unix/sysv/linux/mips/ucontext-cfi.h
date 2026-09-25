/* Unwind information for the MIPS context functions.
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

#define GREG_CFI(n) \
  cfi_offset (n, MCONTEXT_GREGOFF + (n) * MCONTEXT_GREGSZ + MCONTEXT_GREGS)

#if ! defined (__PIC__) || _MIPS_SIM != _ABIO32
# define GP_CFI GREG_CFI (28)
# define GP_RESTORE_CFI cfi_restore (28)
#else
# define GP_CFI
# define GP_RESTORE_CFI
#endif

/* Describe the context whose address is in BASE to the unwinder.  */
#define CONTEXT_CFI(base) \
  cfi_def_cfa (base, 0); \
  GREG_CFI (16); GREG_CFI (17); GREG_CFI (18); GREG_CFI (19); \
  GREG_CFI (20); GREG_CFI (21); GREG_CFI (22); GREG_CFI (23); \
  GP_CFI; GREG_CFI (29); GREG_CFI (30); GREG_CFI (31)

/* The registers hold the new context.  */
#define CONTEXT_REGS_CFI \
  cfi_def_cfa (29, 0); \
  cfi_restore (16); cfi_restore (17); cfi_restore (18); cfi_restore (19); \
  cfi_restore (20); cfi_restore (21); cfi_restore (22); cfi_restore (23); \
  GP_RESTORE_CFI; cfi_restore (29); cfi_restore (30); cfi_restore (31)
