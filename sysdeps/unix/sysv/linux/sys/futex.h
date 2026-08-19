/* Wrappers for the Linux futex system call.
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

#ifndef _SYS_FUTEX_H
#define _SYS_FUTEX_H	1

#include <features.h>
#include <stdint.h>
#include <bits/types.h>
#include <bits/types/clockid_t.h>
#include <bits/types/struct_timespec.h>

#ifndef FUTEX_PRIVATE_FLAG
# define FUTEX_PRIVATE_FLAG	128
#endif

/* Maximum number of entries in the array passed to futex_waitv.  */
#define FUTEX_WAITV_MAX		128

/* Per-waiter flags for futex_waitv, following the kernel definitions
   from <linux/futex.h>.  */
#ifndef FUTEX2_SIZE_U32
# define FUTEX2_SIZE_U32	0x02
#endif
#ifndef FUTEX2_PRIVATE
# define FUTEX2_PRIVATE		FUTEX_PRIVATE_FLAG
#endif

/* One futex to wait on in a futex_waitv call, with the same layout as
   the kernel struct futex_waitv.  */
struct futex_waiter
{
  uint64_t val;			/* Expected value of the futex word.  */
  uint64_t uaddr;		/* Address of the futex word.  */
  uint32_t flags;		/* FUTEX2_* flags for this futex word.  */
  uint32_t __reserved;		/* Must be zero.  */
};

__BEGIN_DECLS

/* If *FUTEXP == EXPECTED block until woken by futex_wake (or spuriously).
   Returns 0 when woken, or -1 and sets errno on failure.  */
extern int futex_wait (uint32_t *__futexp, uint32_t __expected,
		       unsigned int __flags)
     __THROW __nonnull ((1));

/* Like futex_wait, but do not block past the absolute timeout ABSTIME
   measured against CLOCKID (which must be CLOCK_REALTIME or
   CLOCK_MONOTONIC).  If ABSTIME is null, block without a timeout.  */
#ifdef __USE_TIME_BITS64
extern int futex_timedwait (uint32_t *__futexp, uint32_t __expected,
			    clockid_t __clockid,
			    const struct timespec *__abstime,
			    unsigned int __flags)
     __THROW __nonnull ((1));
#endif

/* Wake up to COUNT threads blocked on FUTEXP (INT_MAX to wake all waiters).
   Returns the number of woken threads (zero if no thread was blocked), or
   -1 and sets errno on failure.  */
extern int futex_wake (uint32_t *__futexp, int __count, unsigned int __flags)
     __THROW __nonnull ((1));

/* If *FUTEXP == EXPECTED wake up to NWAKE threads blocked on FUTEXP and
   requeue up to NREQUEUE of the remaining blocked threads to wait on TARGETP
   instead.  Returns the total number of woken and requeued threads, or -1
   and sets errno on failure (EAGAIN if *FUTEXP does not match EXPECTED).  */
extern int futex_requeue (uint32_t *__futexp, uint32_t __expected,
			  int __nwake, uint32_t *__targetp, int __nrequeue,
			  unsigned int __flags)
     __THROW __nonnull ((1, 4));

/* Block on all of the NWAITERS futex words described by WAITERS (at
   most FUTEX_WAITV_MAX entries) until woken on any of them or until
   the absolute timeout ABSTIME measured against CLOCKID (which must
   be CLOCK_REALTIME or CLOCK_MONOTONIC) passes.  If ABSTIME is null,
   block without a timeout.  Each entry checks its own expected value
   and carries its own FUTEX2_* flags (FUTEX2_SIZE_U32, optionally with
   FUTEX2_PRIVATE); the array is passed to the kernel unchanged.  FLAGS
   is the flags argument of the system call, which currently supports
   no flags: it must be zero and is passed to the kernel unchanged.
   The arguments follow the futex_waitv system call order.

   Returns the array index of one of the woken futex words, or -1
   with errno set to:

     EAGAIN     A futex word did not contain its expected value at
		the time of the call.
     EINTR      The wait was interrupted by a signal.
     ETIMEDOUT  ABSTIME has already passed or passed while waiting.
     EINVAL     NWAITERS is zero or greater than FUTEX_WAITV_MAX, a
		futex word is not 4-byte aligned, a flags field is
		invalid, or CLOCKID is invalid.
     ENOSYS     The kernel does not support the futex_waitv system
		call (added in Linux 5.16).

   This function only operates on 64-bit time_t and is only declared
   when time_t is a 64-bit type, so on ABIs where the default time_t
   is 32 bits it requires building with _TIME_BITS=64.  */
#ifdef __USE_TIME_BITS64
extern int futex_waitv (const struct futex_waiter *__waiters,
			unsigned int __nwaiters, unsigned int __flags,
			const struct timespec *__abstime,
			clockid_t __clockid)
     __THROW __nonnull ((1));
#endif

__END_DECLS

#endif /* sys/futex.h */
