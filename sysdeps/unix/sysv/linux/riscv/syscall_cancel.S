/* Cancellable syscall wrapper.  Linux/riscv version.
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
#include <descr-const.h>

/* long int __syscall_cancel_arch (__syscall_arg_t arg1,
				   __syscall_arg_t arg2,
				   __syscall_arg_t arg3,
				   __syscall_arg_t arg4,
				   __syscall_arg_t arg5,
				   __syscall_arg_t arg6,
				   __syscall_arg_t nr,
				   int *cancelhandling)  */

#ifdef SHARED
	.option pic
#else
	.option nopic
#endif

ENTRY (__syscall_cancel_arch)

	.globl __syscall_cancel_arch_start
__syscall_cancel_arch_start:
	lw	t1, 0(a7)
	/* if (*ch & CANCELED_BITMASK)  */
	andi	t1, t1, TCB_CANCELED_BITMASK
	bne	t1, zero, 1f

	/* The arguments are already in place.  */
	mv	a7, a6
	scall

	.globl __syscall_cancel_arch_end
__syscall_cancel_arch_end:
	ret

1:
	addi	sp, sp, -16
	cfi_def_cfa_offset (16)
	REG_S	ra, (16-SZREG)(sp)
	cfi_offset (ra, -SZREG)
	call	__syscall_do_cancel

END (__syscall_cancel_arch)
