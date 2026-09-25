/* Unwind information for the SH context functions.
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

/* Keep the caller r8 on the stack: it holds the context pointer across
   the system call.  */
#define SAVE_R8 \
	mov.l	r8, @-r15; \
	cfi_adjust_cfa_offset (4); \
	cfi_rel_offset (r8, 0)

#define RESTORE_R8 \
	mov.l	@r15+, r8; \
	cfi_adjust_cfa_offset (-4); \
	cfi_restore (r8)

/* Describe the context whose address is in BASE to the unwinder.  */
#define CONTEXT_CFI(base) \
	cfi_def_cfa (base, 0); \
	cfi_offset (r8, oR8); \
	cfi_offset (r9, oR9); \
	cfi_offset (r10, oR10); \
	cfi_offset (r11, oR11); \
	cfi_offset (r12, oR12); \
	cfi_offset (r13, oR13); \
	cfi_offset (r14, oR14); \
	cfi_offset (r15, oR15); \
	cfi_offset (pr, oPR)

/* The registers hold the new context, with N bytes still pushed.  */
#define CONTEXT_REGS_CFI(n) \
	cfi_def_cfa (r15, n); \
	cfi_restore (r8); \
	cfi_restore (r9); \
	cfi_restore (r10); \
	cfi_restore (r11); \
	cfi_restore (r12); \
	cfi_restore (r13); \
	cfi_restore (r14); \
	cfi_restore (r15); \
	cfi_restore (pr)
