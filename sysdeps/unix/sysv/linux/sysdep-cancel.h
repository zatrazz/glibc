/* Single-thread optimization definitions.  Linux version.
   Copyright (C) 2017-2026 Free Software Foundation, Inc.

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

#ifndef _SYSDEP_CANCEL_H
#define _SYSDEP_CANCEL_H

#include <sysdep.h>
#include "pthreadP.h"

#ifdef HAVE_CANCELABLE_SYSCALL_WITH_7_ARGS
# define __SYSCALL_CANCEL_A7_DECL(a7)	__syscall_arg_t __sc_a7 = __SSC (a7);
# define __SYSCALL_CANCEL_A7		, __sc_a7
#else
# define __SYSCALL_CANCEL_A7_DECL(a7)
# define __SYSCALL_CANCEL_A7
#endif

/* Issue the cancellable syscall NR through __syscall_cancel_arch, which
   contains the global markers checked by the SIGCANCEL handler, for the
   thread PD with cancellation enabled.  */
static __always_inline long int
syscall_cancel_arch (__syscall_arg_t a1, __syscall_arg_t a2,
		     __syscall_arg_t a3, __syscall_arg_t a4,
		     __syscall_arg_t a5, __syscall_arg_t a6
		     __SYSCALL_CANCEL7_ARCH_ARG_DEF, __syscall_arg_t nr,
		     struct pthread *pd)
{
  long int result = __syscall_cancel_arch (&pd->cancelhandling, nr, a1, a2,
					   a3, a4, a5, a6
					   __SYSCALL_CANCEL7_ARCH_ARG7);

  /* If the cancellable syscall was interrupted by SIGCANCEL and it has no
     side-effect, cancel the thread if cancellation is enabled.
     The behaviour here assumes that EINTR is returned only if there are no
     visible side effects.  POSIX Issue 7 has not yet provided any stronger
     language for close, and in theory the close syscall could return EINTR
     and leave the file descriptor open (conforming and leaks).  It expects
     that no such kernel is used with glibc.  */
  if (result == -EINTR
      && cancel_enabled_and_canceled (
	  atomic_load_relaxed (&pd->cancelhandling)))
    __syscall_do_cancel ();

  return result;
}

/* Used by the INTERNAL_SYSCALL_CANCEL macro, check for cancellation and
   evaluate to the syscall value or its negative error code.  DIRECT is the
   syscall issued directly, used if the process is single-threaded, if
   cancellation is disabled, or if the thread is exiting (to avoid acting
   on cancellation while running the cleanup handlers).  The cancelhandling
   is only loaded for the multi-threaded case.  */
#define __INTERNAL_SYSCALL_CANCEL_IMPL(direct, nr, a1, a2, a3, a4, a5, a6, \
				       a7)				   \
  ({									   \
    __syscall_arg_t __sc_a1 = __SSC (a1);				   \
    __syscall_arg_t __sc_a2 = __SSC (a2);				   \
    __syscall_arg_t __sc_a3 = __SSC (a3);				   \
    __syscall_arg_t __sc_a4 = __SSC (a4);				   \
    __syscall_arg_t __sc_a5 = __SSC (a5);				   \
    __syscall_arg_t __sc_a6 = __SSC (a6);				   \
    __SYSCALL_CANCEL_A7_DECL (a7)					   \
    long int __sc_ret;							   \
    if (SINGLE_THREAD_P)						   \
      __sc_ret = (direct);						   \
    else								   \
      {									   \
	struct pthread *__sc_pd = THREAD_SELF;				   \
	int __sc_ch = atomic_load_relaxed (&__sc_pd->cancelhandling);	   \
	if (cancel_disabled_or_exiting (__sc_ch))			   \
	  __sc_ret = (direct);						   \
	else								   \
	  __sc_ret = syscall_cancel_arch (__sc_a1, __sc_a2, __sc_a3,	   \
					  __sc_a4, __sc_a5, __sc_a6	   \
					  __SYSCALL_CANCEL_A7, nr,	   \
					  __sc_pd);			   \
      }									   \
    __sc_ret;								   \
  })

/* Used by the SYSCALL_CANCEL macro, return the syscall expected success
   value R (usually 0) or, in case of failure, -1 and sets errno to syscall
   return value.  */
static __always_inline long int
syscall_cancel_ret (long int r)
{
  /* Set errno out of line, so both the direct and the cancellable syscall
     paths can tail call it instead of each one having a copy.  */
  if (__glibc_unlikely (INTERNAL_SYSCALL_ERROR_P (r)))
    return __syscall_cancel_error (r);
  return r;
}

#endif
