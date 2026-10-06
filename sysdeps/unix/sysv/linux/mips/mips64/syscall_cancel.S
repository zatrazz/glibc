/* Cancellable syscall wrapper.  Linux/mips64 version.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.
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
   <http://www.gnu.org/licenses/>.  */

#include <sysdep.h>
#include <sys/asm.h>
#include <descr-const.h>

/* long int __syscall_cancel_arch (__syscall_arg_t arg1,
				   __syscall_arg_t arg2,
				   __syscall_arg_t arg3,
				   __syscall_arg_t arg4,
				   __syscall_arg_t arg5,
				   __syscall_arg_t arg6,
				   __syscall_arg_t nr,
				   int *cancelhandling)  */

#define FRAME_SIZE 16

	.text
NESTED (__syscall_cancel_arch, FRAME_SIZE, ra)
	.mask	0x90000000, -SZREG
	.fmask	0x00000000, 0
	LONG_ADDIU	sp, sp, -FRAME_SIZE
	cfi_def_cfa_offset (FRAME_SIZE)
	sd		gp, 0(sp)
	cfi_offset (gp, -16)
	lui		gp, %hi(%neg(%gp_rel(__syscall_cancel_arch)))
	LONG_ADDU	gp, gp, t9
	sd		ra, 8(sp)
	cfi_offset (ra, -8)
	LONG_ADDIU	gp, gp, %lo(%neg(%gp_rel(__syscall_cancel_arch)))

	.global __syscall_cancel_arch_start
__syscall_cancel_arch_start:

	lw		v0, 0(a7)
	andi		v0, v0, TCB_CANCELED_BITMASK
	bne		v0, zero, 2f

	/* The arguments are already in place (a0 to a5).  */
	.set noreorder
	move		v0, a6
	syscall
	.set reorder

	.global __syscall_cancel_arch_end
__syscall_cancel_arch_end:

	.set noreorder
	.set nomacro
#if __mips_isa_rev >= 6
	beqzc	$7, 1f
#else
	bnel	a3, zero, 1f
#endif
	SUBU	v0, zero, v0
	.set macro
	.set reorder

1:
	ld		ra, 8(sp)
	ld		gp, 0(sp)

	.set	noreorder
	.set	nomacro
	jr		ra
	LONG_ADDIU	sp, sp, FRAME_SIZE
	.set	macro
	.set	reorder

	cfi_remember_state
	cfi_def_cfa_offset (0)
	cfi_restore (gp)
	cfi_restore (ra)
	.align	3
2:
	cfi_restore_state
	LONG_L		t9, %got_disp(__syscall_do_cancel)(gp)
	.reloc	3f, R_MIPS_JALR, __syscall_do_cancel
3:	jalr		t9
END (__syscall_cancel_arch)
