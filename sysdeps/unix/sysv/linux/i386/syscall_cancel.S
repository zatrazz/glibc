/* Cancellable syscall wrapper.  Linux/i686 version.
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

/* long int [eax] __syscall_cancel_arch (long int arg1 [SP],
					 long int arg2 [SP+4],
					 long int arg3 [SP+8],
					 long int arg4 [SP+12],
					 long int arg5 [SP+16],
					 long int arg6 [SP+20],
					 long int nr   [SP+24],
					 int *cancelhandling [SP+28])  */

ENTRY (__syscall_cancel_arch)
	pushl %ebp
	cfi_def_cfa_offset (8)
	cfi_offset (ebp, -8)
	pushl %edi
	cfi_def_cfa_offset (12)
	cfi_offset (edi, -12)
	pushl %esi
	cfi_def_cfa_offset (16)
	cfi_offset (esi, -16)
	pushl %ebx
	cfi_def_cfa_offset (20)
	cfi_offset (ebx, -20)

	.global __syscall_cancel_arch_start
__syscall_cancel_arch_start:

	/* if (*cancelhandling & CANCELED_BITMASK)
	     __syscall_do_cancel()  */
	movl	48(%esp), %eax
	testb	$TCB_CANCELED_BITMASK, (%eax)
	jne     1f

	/* Issue a 6 argument syscall, the nr [%eax] being the syscall
	   number.  */
	movl    20(%esp), %ebx
	movl    24(%esp), %ecx
	movl    28(%esp), %edx
	movl    32(%esp), %esi
	movl    36(%esp), %edi
	movl    40(%esp), %ebp
	movl    44(%esp), %eax

	/* We can not use the vDSO helper for syscall (__kernel_vsyscall)
	   because the returned PC from kernel will point to the vDSO page
	   instead of the expected __syscall_cancel_arch_{start,end}
	   marks.  */
	int	$0x80

	.global __syscall_cancel_arch_end
__syscall_cancel_arch_end:

	popl %ebx
	cfi_restore (ebx)
	cfi_def_cfa_offset (16)
	popl %esi
	cfi_restore (esi)
	cfi_def_cfa_offset (12)
	popl %edi
	cfi_restore (edi)
	cfi_def_cfa_offset (8)
	popl %ebp
	cfi_restore (ebp)
	cfi_def_cfa_offset (4)
        ret

1:
	/* Although the __syscall_do_cancel do not return, we need to stack
	   being set correctly for unwind.  */
	popl %ebx
	cfi_restore (ebx)
	cfi_def_cfa_offset (16)
	popl %esi
	cfi_restore (esi)
	cfi_def_cfa_offset (12)
	popl %edi
	cfi_restore (edi)
	cfi_def_cfa_offset (8)
	popl %ebp
	cfi_restore (ebp)
	cfi_def_cfa_offset (4)
	jmp __syscall_do_cancel

END (__syscall_cancel_arch)
