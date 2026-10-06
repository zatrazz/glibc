/* Cancellable syscall wrapper.  Linux/loongarch version.
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

ENTRY (__syscall_cancel_arch)

	.global __syscall_cancel_arch_start
__syscall_cancel_arch_start:

	/* if (*cancelhandling & CANCELED_BITMASK)
	     __syscall_do_cancel()  */
	ld.w	t0, a7, 0
	andi	t0, t0, TCB_CANCELED_BITMASK
	bne	t0, zero, 1f

	/* Issue a 6 argument syscall, the arguments are already in place.  */
	move	a7, a6
	syscall 0

	.global __syscall_cancel_arch_end
__syscall_cancel_arch_end:
	jr	ra
1:
	b	__syscall_do_cancel

END (__syscall_cancel_arch)
