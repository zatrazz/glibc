/* Cancellable syscall wrapper.  Linux/x86_64 version.
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

/* long int [rax] __syscall_cancel_arch (__syscall_arg_t arg1 [%rdi],
					 __syscall_arg_t arg2 [%rsi],
					 __syscall_arg_t arg3 [%rdx],
					 __syscall_arg_t arg4 [%rcx],
					 __syscall_arg_t arg5 [%r8],
					 __syscall_arg_t arg6 [%r9],
					 __syscall_arg_t nr   [SP+8],
					 volatile int *cancelhandling [SP+16])  */

ENTRY (__syscall_cancel_arch)
	.globl __syscall_cancel_arch_start
__syscall_cancel_arch_start:

	/* if (*cancelhandling & CANCELED_BITMASK)
	     __syscall_do_cancel()  */
	LP_OP(mov) 16(%rsp), %RAX_LP
	testb	$TCB_CANCELED_BITMASK, (%rax)
	jne	__syscall_do_cancel

	/* Issue a 6 argument syscall, the arguments are already in place
	   except for arg4 that the kernel expects in %r10.  */
	mov	%rcx, %r10
	mov	8(%rsp), %rax
	syscall

	.globl __syscall_cancel_arch_end
__syscall_cancel_arch_end:
	ret
END (__syscall_cancel_arch)
