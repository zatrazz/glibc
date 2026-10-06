/* Cancellable syscall wrapper.  Linux/arm version.
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

/* long int [r0] __syscall_cancel_arch (long int arg1 [r0],
					long int arg2 [r1],
					long int arg3 [r2],
					long int arg4 [r3],
					long int arg5 [SP],
					long int arg6 [SP+4],
					long int nr   [SP+8],
					int *cancelhandling [SP+12])  */

	.syntax unified

ENTRY (__syscall_cancel_arch)
	.fnstart
	mov	ip, sp
	stmfd	sp!, {r4, r5, r6, r7, lr}
	.save	{r4, r5, r6, r7, lr}

	cfi_adjust_cfa_offset (20)
	cfi_rel_offset (r4, 0)
	cfi_rel_offset (r5, 4)
	cfi_rel_offset (r6, 8)
	cfi_rel_offset (r7, 12)
	cfi_rel_offset (lr, 16)

	.globl __syscall_cancel_arch_start
__syscall_cancel_arch_start:

	/* if (*cancelhandling & CANCELED_BITMASK)
	     __syscall_do_cancel()  */
	ldr	r6, [ip, #12]
	ldr	r6, [r6]
	tst	r6, #TCB_CANCELED_BITMASK
	bne	1f

	/* Issue a 6 argument syscall, arg1 to arg4 are already in place.  */
	ldmfd	ip, {r4, r5, r7}
	svc	0x0

	.globl __syscall_cancel_arch_end
__syscall_cancel_arch_end:
	ldmfd	sp!, {r4, r5, r6, r7, lr}
	cfi_adjust_cfa_offset (-20)
        cfi_restore (r4)
        cfi_restore (r5)
        cfi_restore (r6)
        cfi_restore (r7)
        cfi_restore (lr)
	BX (lr)

1:
	ldmfd	sp!, {r4, r5, r6, r7, lr}
	b	__syscall_do_cancel
	.fnend
END (__syscall_cancel_arch)
