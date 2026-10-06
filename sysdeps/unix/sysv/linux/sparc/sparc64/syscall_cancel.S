/* Cancellable syscall wrapper.  Linux/sparc64 version.
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

	.register       %g2, #scratch

/* long int __syscall_cancel_arch (long int arg1,
				   long int arg2,
				   long int arg3,
				   long int arg4,
				   long int arg5,
				   long int arg6,
				   long int nr,
				   int *cancelhandling)

   The arguments are already in place (%o0 to %o5), so there is no need
   for a register window.  */

ENTRY (__syscall_cancel_arch)

	.globl __syscall_cancel_arch_start
__syscall_cancel_arch_start:

	/* if (*cancelhandling & CANCELED_BITMASK)
	     __syscall_do_cancel()  */
	ldx	[%sp + STACK_BIAS + 184], %g2
	lduw	[%g2], %g2
	andcc	%g2, TCB_CANCELED_BITMASK, %g0
	bne,pn	%xcc, 2f
	 ldx	[%sp + STACK_BIAS + 176], %g1
	ta	0x6d

	.global __syscall_cancel_arch_end
__syscall_cancel_arch_end:

	bcc,pt	%xcc, 1f
	 nop
	sub	%g0, %o0, %o0
1:
	retl
	 nop

2:
	/* Tail call, keep %o7 as the return address for unwinding.  */
	mov	%o7, %g1
	call	__syscall_do_cancel, 0
	 mov	%g1, %o7

END (__syscall_cancel_arch)
