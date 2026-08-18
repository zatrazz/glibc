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

/* The futex word is only used by threads of the calling process.  */
#define FUTEX_FLAG_PRIVATE	0x00u
/* The futex word may be used by multiple processes through a shared
   memory mapping.  */
#define FUTEX_FLAG_SHARED	0x01u

__BEGIN_DECLS

/* If *FUTEXP == EXPECTED block until woken by futex_wake (or spuriously).
   Returns 0 when woken, or a positive error code.  */
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
   Returns the number of woken threads or zero if no thread was blocked.  */
extern int futex_wake (uint32_t *__futexp, int __count, unsigned int __flags)
     __THROW __nonnull ((1));

/* If *FUTEXP == EXPECTED wake up to NWAKE threads blocked on FUTEXP and
   requeue up to NREQUEUE of the remaining blocked threads to wait on TARGETP
   instead.  Returns the total number of woken and requeued threads, or a
   negated error code.  */
extern int futex_requeue (uint32_t *__futexp, uint32_t __expected,
			  int __nwake, uint32_t *__targetp, int __nrequeue,
			  unsigned int __flags)
     __THROW __nonnull ((1, 4));

__END_DECLS

#endif /* sys/futex.h */
