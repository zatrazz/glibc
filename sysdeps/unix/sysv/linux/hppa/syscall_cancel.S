/* Cancellable syscall wrapper.  Linux/hppa version.
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
   License along with the GNU C Library.  If not, see
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

	.text
ENTRY(__syscall_cancel_arch)
	stw	%r2,-20(%r30)
	ldo	128(%r30),%r30
	cfi_def_cfa_offset (-128)
	cfi_offset (2, -20)
	stw	%r4,-104(%r30)
#ifdef __PIC__
	stw	%r19,-32(%r30)
#endif
	cfi_offset (4, 24)

	.global __syscall_cancel_arch_start
__syscall_cancel_arch_start:
	/* arg1 to arg4 are already in place (%r26 to %r23), the remaining
	   ones are at the caller stack argument area (-52 to -64).  */
	ldw	-192(%r30),%r20
	ldw	0(%r20),%r20
	bb,<	%r20,31-TCB_CANCELED_BIT,1f
	ldw	-180(%r30),%r22
	ldw	-184(%r30),%r21
	ldw	-188(%r30),%r28
	copy	%r19, %r4
	ble	0x100(%sr2, %r0)

	.global __syscall_cancel_arch_end
__syscall_cancel_arch_end:

	/* On a syscall restart the kernel executes the ble again and only
	   accepts ldi, nop, or copy to %r20 in its delay slot.  */
	copy	%r28,%r20
	copy	%r4,%r19

	ldw	-148(%r30),%r2
	ldw	-104(%r30),%r4
	bv	%r0(%r2)
	ldo	-128(%r30),%r30
1:
	bl	__syscall_do_cancel,%r2
	nop
	nop

END(__syscall_cancel_arch)
