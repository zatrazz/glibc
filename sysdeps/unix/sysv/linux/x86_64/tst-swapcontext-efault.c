/* swapcontext failing with EFAULT preserves callee-saved registers.
   Copyright (C) 2026 Free Software Foundation, Inc.
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
   <https://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>
#include <support/check.h>
#include <support/xunistd.h>

/* Call swapcontext (OUCP, UCP) with r12 set to MAGIC and store the r12
   seen on return in *R12.  */
long int call_swapcontext (ucontext_t *oucp, const ucontext_t *ucp,
			   uint64_t *r12);
#define MAGIC 0x1234567887654321
asm (".text\n"
     ".type call_swapcontext, @function\n"
     "call_swapcontext:\n"
     "	pushq	%r12\n"
     "	pushq	%rbx\n"
     "	subq	$8, %rsp\n"
     "	movq	%rdx, %rbx\n"
     "	movabsq	$0x1234567887654321, %r12\n"
     "	call	swapcontext@PLT\n"
     "	movq	%r12, (%rbx)\n"
     "	addq	$8, %rsp\n"
     "	popq	%rbx\n"
     "	popq	%r12\n"
     "	ret\n"
     ".size call_swapcontext, .-call_swapcontext\n");

static int
do_test (void)
{
  static ucontext_t oucp;
  long int page = xsysconf (_SC_PAGESIZE);
  /* rt_sigprocmask fails on it before swapcontext reads it.  */
  void *ucp = xmmap (NULL, page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1);

  uint64_t r12 = 0;
  errno = 0;
  TEST_COMPARE (call_swapcontext (&oucp, ucp, &r12), -1);
  TEST_COMPARE (errno, EFAULT);
  TEST_COMPARE (r12, MAGIC);

  xmunmap (ucp, page);
  return 0;
}

#include <support/test-driver.c>
