/* Cancellable syscall wrapper.  Linux/mips32 version.
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
				   __syscall_arg_t arg7,
				   __syscall_arg_t nr,
				   int *cancelhandling)

   The o32 syscall ABI takes the arguments 5 to 7 from the same stack
   slots used for the function call (16, 20, and 24 from sp), so all the
   syscall arguments are already in place.  */

ENTRY (__syscall_cancel_arch)

	.globl __syscall_cancel_arch_start
__syscall_cancel_arch_start:

	lw	v0, 32(sp)
	lw	v0, 0(v0)
	andi	v0, v0, TCB_CANCELED_BITMASK
	bne	v0, zero, 2f

	lw	v0, 28(sp)
	syscall

	.globl __syscall_cancel_arch_end
__syscall_cancel_arch_end:
	beq	a3, zero, 1f
	subu	v0, zero, v0
1:
	jr	ra

2:
	/* The gp is still the one set up by the libc caller.  */
#ifdef __PIC__
	PTR_LA	t9, __syscall_do_cancel
	jr	t9
#else
	j	__syscall_do_cancel
#endif

END (__syscall_cancel_arch)
