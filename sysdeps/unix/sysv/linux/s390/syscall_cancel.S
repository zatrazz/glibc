/* Cancellable syscall wrapper.  Linux/s390x version.
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
	stg	%r7,56(%r15)
	cfi_offset (%r7, -104)

	.globl __syscall_cancel_arch_start
__syscall_cancel_arch_start:
	/* if (*cancelhandling & CANCELED_BITMASK)
	     __syscall_do_cancel()  */
	lg	%r1,176(%r15)
	tm	3(%r1),TCB_CANCELED_BITMASK
	jne	1f

	/* Issue a 6 argument syscall, arg1 to arg5 are already in place
	   and the nr [%r1] being the syscall number.  */
	lg	%r1,168(%r15)
	lg	%r7,160(%r15)
	svc	0

	.globl __syscall_cancel_arch_end
__syscall_cancel_arch_end:
	lg	%r7,56(%r15)
	cfi_remember_state
	cfi_restore (%r7)
	br	%r14
1:
	cfi_restore_state
	jg	__syscall_do_cancel
END (__syscall_cancel_arch)
