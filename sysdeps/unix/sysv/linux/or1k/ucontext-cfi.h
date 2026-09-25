/* Unwind information for the OpenRISC context functions.
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

/* Describe the context whose address is in r30 to the unwinder.  */
#define CONTEXT_CFI \
	cfi_def_cfa (30, 0); \
	cfi_offset (1, UCONTEXT_MCONTEXT + 1*4); \
	cfi_offset (2, UCONTEXT_MCONTEXT + 2*4); \
	cfi_offset (9, UCONTEXT_MCONTEXT + 9*4); \
	cfi_offset (14, UCONTEXT_MCONTEXT + 14*4); \
	cfi_offset (16, UCONTEXT_MCONTEXT + 16*4); \
	cfi_offset (18, UCONTEXT_MCONTEXT + 18*4); \
	cfi_offset (20, UCONTEXT_MCONTEXT + 20*4); \
	cfi_offset (22, UCONTEXT_MCONTEXT + 22*4); \
	cfi_offset (24, UCONTEXT_MCONTEXT + 24*4); \
	cfi_offset (26, UCONTEXT_MCONTEXT + 26*4); \
	cfi_offset (28, UCONTEXT_MCONTEXT + 28*4); \
	cfi_offset (30, UCONTEXT_MCONTEXT + 30*4)

/* The registers hold the new context.  */
#define CONTEXT_REGS_CFI \
	cfi_def_cfa (1, 0); \
	cfi_restore (1); \
	cfi_restore (2); \
	cfi_restore (9); \
	cfi_restore (14); \
	cfi_restore (16); \
	cfi_restore (18); \
	cfi_restore (20); \
	cfi_restore (22); \
	cfi_restore (24); \
	cfi_restore (26); \
	cfi_restore (28); \
	cfi_restore (30)
