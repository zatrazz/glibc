/* Common definition for pthread_{timed,try}join{_np}.
   Copyright (C) 2017-2025 Free Software Foundation, Inc.
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

#include "pthreadP.h"
#include <atomic.h>
#include <stap-probe.h>
#include <time.h>
#include <futex-internal.h>

/* Check for a possible deadlock situation where the threads are waiting for
   each other to finish.  Note that this is a "may" error.  To be 100% sure we
   catch this error we would have to lock the data structures but it is not
   necessary.  In the unlikely case that two threads are really caught in this
   situation they will deadlock.  It is the programmer's problem to figure
   this out.  */
static inline bool
check_for_deadlock (struct pthread *pd)
{
  struct pthread *self = THREAD_SELF;
  return ((pd == self
	   || (atomic_load_acquire (&self->joinstate) == THREAD_STATE_DETACHED
	       && (pd->cancelhandling
		   & (CANCELING_BITMASK | CANCELED_BITMASK | EXITING_BITMASK
		      | TERMINATED_BITMASK)) == 0))
	  && !cancel_enabled_and_canceled (self->cancelhandling));
}

int
__pthread_clockjoin_ex (pthread_t threadid, void **thread_return,
                        clockid_t clockid,
                        const struct __timespec64 *abstime)
{
  struct pthread *pd = (struct pthread *) threadid;

  /* Make sure the clock and time specified are valid.  */
  if (abstime
      && __glibc_unlikely (!futex_abstimed_supported_clockid (clockid)
			   || ! valid_nanoseconds (abstime->tv_nsec)))
    return EINVAL;

  LIBC_PROBE (pthread_join, 1, threadid);

  int result = 0;
  unsigned int state;
  while ((state = atomic_load_acquire (&pd->joinstate))
	 != THREAD_STATE_EXITED)
    {
      if (check_for_deadlock (pd))
	return EDEADLK;

      /* POSIX states calling pthread_join on a non joinable thread is
	 undefined.  However, if PD is still in the cache we can warn
         the caller.  */
      if (state == THREAD_STATE_DETACHED)
	return EINVAL;

      /* pthread_join is a cancellation entrypoint and we use the same
         rationale for pthread_timedjoin_np.

	 The kernel notifies a process which uses CLONE_CHILD_CLEARTID via
	 a memory zeroing and futex wake-up when the process terminates.
	 The futex operation is not private.  */
      int ret = __futex_abstimed_wait_cancelable64 (&pd->joinstate, state,
						    clockid, abstime,
						    LLL_SHARED);
      if (ret == ETIMEDOUT || ret == EOVERFLOW)
	{
	  result = ret;
	  break;
	}
    }

  void *pd_result = pd->result;
  if (__glibc_likely (result == 0))
    {
      if (thread_return != NULL)
	*thread_return = pd_result;

      /* Free the TCB.  */
      __nptl_free_tcb (pd);
    }

  LIBC_PROBE (pthread_join_ret, 3, threadid, result, pd_result);

  return result;
}
