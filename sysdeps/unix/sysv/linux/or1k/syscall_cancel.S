/* Cancellable syscall wrapper.  Linux/or1k version.
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
	l.addi	r1, r1, -4
	cfi_def_cfa_offset (4)
	l.sw	0(r1), r9
	cfi_offset (9, -4)

	.global __syscall_cancel_arch_start
__syscall_cancel_arch_start:

	/* if (*cancelhandling & CANCELED_BITMASK)
	     __syscall_do_cancel()  */
	l.lwz	r17, 8(r1)
	l.lwz	r17, 0(r17)
	l.andi	r17, r17, TCB_CANCELED_BITMASK
	l.sfnei	r17, 0
	l.bf	1f
	 l.lwz	r11, 4(r1)

	/* Issue a 6 argument syscall, the arguments are already in place.  */
	l.sys	1
	 l.nop

	.global __syscall_cancel_arch_end
__syscall_cancel_arch_end:

	l.lwz	r9, 0(r1)
	l.jr	r9
	l.addi	r1, r1, 4
	cfi_remember_state
	cfi_def_cfa_offset (0)
	cfi_restore (9)
1:
	cfi_restore_state
	l.jal	__syscall_do_cancel
	 l.nop
END (__syscall_cancel_arch)
