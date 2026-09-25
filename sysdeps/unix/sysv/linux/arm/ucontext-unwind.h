/* Unwind information for the ARM context functions.
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

#include "ucontext_i.h"

/* The EHABI unwinder looks up PC - 2 even for an interrupted frame, so
   a region starts at the instruction that changes the frame, and the
   entry point needs an instruction of its region before it.  */
#define EHABI_ENTRY(name) \
	eabi_fnstart; \
	nop; \
	ENTRY (name)

/* Start a new EHABI region.  The unwinder then uses the default rule,
   return to LR with SP unchanged, until another rule is given.  */
#define EHABI_REGION \
	eabi_fnend; \
	eabi_fnstart

/* The caller context is in the ucontext_t, OFF bytes before the address
   in register number REG: pop R4-R14 (SP and LR included) from its
   mcontext and return to LR.  */
#define EHABI_CONTEXT(reg, off) \
	.unwind_raw 0, 0x90 | (reg), (MCONTEXT_ARM_R4 - (off) - 4) / 4, \
	  0x87, 0xff

/* Likewise for DWARF, with REG pointing to OFF bytes into the
   ucontext_t.  The CFA is the saved LR.  */
#define CFI_CONTEXT(reg, off) \
	cfi_def_cfa (reg, MCONTEXT_ARM_LR - (off)); \
	cfi_offset (r4, MCONTEXT_ARM_R4 - MCONTEXT_ARM_LR); \
	cfi_offset (r5, MCONTEXT_ARM_R4 + 4 - MCONTEXT_ARM_LR); \
	cfi_offset (r6, MCONTEXT_ARM_R4 + 8 - MCONTEXT_ARM_LR); \
	cfi_offset (r7, MCONTEXT_ARM_R4 + 12 - MCONTEXT_ARM_LR); \
	cfi_offset (r8, MCONTEXT_ARM_R4 + 16 - MCONTEXT_ARM_LR); \
	cfi_offset (r9, MCONTEXT_ARM_R4 + 20 - MCONTEXT_ARM_LR); \
	cfi_offset (r10, MCONTEXT_ARM_R4 + 24 - MCONTEXT_ARM_LR); \
	cfi_offset (r11, MCONTEXT_ARM_R4 + 28 - MCONTEXT_ARM_LR); \
	cfi_offset (r13, MCONTEXT_ARM_SP - MCONTEXT_ARM_LR); \
	cfi_offset (r14, 0)
