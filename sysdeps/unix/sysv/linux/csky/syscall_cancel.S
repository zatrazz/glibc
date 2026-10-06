/* Cancellable syscall wrapper.  Linux/csky version.
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
# define STACK_ADJ 4
#else
# define STACK_ADJ 0
#endif

/* Offset of the stack arguments (arg5, arg6, nr, and cancelhandling)
   after the register save area.  */
#define STACK_ARGS (16 + STACK_ADJ)

ENTRY (__syscall_cancel_arch)
	subi	sp, sp, 16 + STACK_ADJ
	cfi_def_cfa_offset (16 + STACK_ADJ)
#ifdef SHARED
	st.w	gb, (sp, 16)
	lrw	t1, 1f@GOTPC
	cfi_offset (gb, -4)
	grs	gb, 1f
1:
#endif
	st.w	lr, (sp, 12)
	st.w	l3, (sp, 8)
	st.w	l1, (sp, 4)
	st.w	l0, (sp, 0)
#ifdef SHARED
	addu	gb, gb, t1
#endif
	cfi_offset (lr, -( 4 + STACK_ADJ))
	cfi_offset (l3, -( 8 + STACK_ADJ))
	cfi_offset (l1, -(12 + STACK_ADJ))
	cfi_offset (l0, -(16 + STACK_ADJ))

	.globl __syscall_cancel_arch_start
__syscall_cancel_arch_start:
	ld.w	t0, (sp, STACK_ARGS + 12)
	ld.w	t0, (t0, 0)
	andi	t0, t0, TCB_CANCELED_BITMASK
	jbnez	t0, 2f
	/* arg1 to arg4 are already in place.  */
	ld.w	l0, (sp, STACK_ARGS + 0)
	ld.w	l1, (sp, STACK_ARGS + 4)
	ld.w	l3, (sp, STACK_ARGS + 8)
	trap	0

	.globl __syscall_cancel_arch_end
__syscall_cancel_arch_end:
	cfi_remember_state
#ifdef SHARED
	ld.w	gb, (sp, 16)
	cfi_restore (gb)
#endif
	ld.w	lr, (sp, 12)
	cfi_restore (lr)
	ld.w	l3, (sp, 8)
	cfi_restore (l3)
	ld.w	l1, (sp, 4)
	cfi_restore (l1)
	ld.w	l0, (sp, 0)
	cfi_restore (l0)
	addi	sp, sp, 16 + STACK_ADJ
	cfi_def_cfa_offset (0)
	rts

2:
	cfi_restore_state
#ifdef SHARED
	lrw	a3, __syscall_do_cancel@GOTOFF
	addu	a3, a3, gb
	jsr	a3
#else
	jbsr	__syscall_do_cancel
#endif
END (__syscall_cancel_arch)
