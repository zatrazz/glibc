/* Cancellable syscall wrapper.  Linux/microblaze version.
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

/* long int __syscall_cancel_arch (long int arg1,
				   long int arg2,
				   long int arg3,
				   long int arg4,
				   long int arg5,
				   long int arg6,
				   long int nr,
				   int *cancelhandling)  */

ENTRY (__syscall_cancel_arch)

	.globl __syscall_cancel_arch_start
__syscall_cancel_arch_start:

	lwi	r3,r1,32
	lwi	r3,r3,0
	andi	r3,r3,TCB_CANCELED_BITMASK
	bneid	r3,1f
	lwi	r12,r1,28

	/* The arguments are already in place (r5 to r10).  */
	brki	r14,8

	.globl __syscall_cancel_arch_end
__syscall_cancel_arch_end:

	nop
	rtsd	r15,8
	nop

1:
	brlid	r15, __syscall_do_cancel
	nop

END (__syscall_cancel_arch)
