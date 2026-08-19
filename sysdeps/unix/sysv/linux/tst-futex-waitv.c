/* Test for futex_waitv.
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
#include <limits.h>
#include <stdint.h>
#include <sys/futex.h>
#include <support/check.h>
#include <support/test-driver.h>

/* futex_waitv is only declared when time_t is a 64-bit type.  */
#ifdef __USE_TIME_BITS64

# include <support/timespec.h>
# include <support/xthread.h>
# include <support/xtime.h>

static uint32_t ftx1;
static uint32_t ftx2;
static int waiter_result;

static void *
waiter_tf (void *closure)
{
  struct futex_waiter waiters[] =
    {
      { .val = 0, .uaddr = (uintptr_t) &ftx1,
	.flags = FUTEX2_SIZE_U32 | FUTEX_PRIVATE_FLAG },
      { .val = 0, .uaddr = (uintptr_t) &ftx2,
	.flags = FUTEX2_SIZE_U32 | FUTEX_PRIVATE_FLAG },
    };
  int r;
  do
    r = futex_waitv (waiters, 2, 0, NULL, CLOCK_MONOTONIC);
  while (r == -1 && errno == EINTR);
  TEST_VERIFY (r >= 0);
  __atomic_store_n (&waiter_result, r + 1, __ATOMIC_RELEASE);
  return NULL;
}

static int
do_test (void)
{
  uint32_t word = 5;
  struct futex_waiter waiter =
    { .val = 4, .uaddr = (uintptr_t) &word,
      .flags = FUTEX2_SIZE_U32 | FUTEX_PRIVATE_FLAG };

  /* Use a value mismatch to probe for kernel support (Linux 5.16).  */
  {
    int r = futex_waitv (&waiter, 1, 0, NULL, CLOCK_MONOTONIC);
    TEST_COMPARE (r, -1);
    if (errno == ENOSYS)
      FAIL_UNSUPPORTED ("kernel does not support the futex_waitv syscall");
    TEST_COMPARE (errno, EAGAIN);
  }

  /* Nonzero syscall flags are rejected by the kernel.  */
  TEST_COMPARE (futex_waitv (&waiter, 1, ~0u, NULL, CLOCK_MONOTONIC), -1);
  TEST_COMPARE (errno, EINVAL);

  /* Invalid number of waiters and invalid per-waiter flags.  */
  TEST_COMPARE (futex_waitv (&waiter, 0, 0, NULL, CLOCK_MONOTONIC), -1);
  TEST_COMPARE (errno, EINVAL);
  TEST_COMPARE (futex_waitv (&waiter, FUTEX_WAITV_MAX + 1, 0, NULL,
			     CLOCK_MONOTONIC), -1);
  TEST_COMPARE (errno, EINVAL);
  {
    struct futex_waiter bad =
      { .val = 5, .uaddr = (uintptr_t) &word, .flags = ~0u };
    TEST_COMPARE (futex_waitv (&bad, 1, 0, NULL, CLOCK_MONOTONIC), -1);
    TEST_COMPARE (errno, EINVAL);
  }

  /* Invalid clock.  */
  {
    struct timespec ts = make_timespec (0, 0);
    waiter.val = 5;
    TEST_COMPARE (futex_waitv (&waiter, 1, 0, &ts, CLOCK_PROCESS_CPUTIME_ID),
		  -1);
    TEST_COMPARE (errno, EINVAL);
  }

  /* Timeouts in the past, including negative tv_sec.  */
  {
    struct timespec ts = make_timespec (0, 0);
    TEST_COMPARE (futex_waitv (&waiter, 1, 0, &ts, CLOCK_MONOTONIC), -1);
    TEST_COMPARE (errno, ETIMEDOUT);
    ts = make_timespec (-1, 0);
    TEST_COMPARE (futex_waitv (&waiter, 1, 0, &ts, CLOCK_REALTIME), -1);
    TEST_COMPARE (errno, ETIMEDOUT);
  }

  /* An actual short wait on both supported clocks.  */
  for (int i = 0; i < 2; i++)
    {
      clockid_t clockid = i == 0 ? CLOCK_REALTIME : CLOCK_MONOTONIC;
      struct timespec timeout
	= timespec_add (xclock_now (clockid), make_timespec (0, 100000000));
      TEST_COMPARE (futex_waitv (&waiter, 1, 0, &timeout, clockid), -1);
      TEST_COMPARE (errno, ETIMEDOUT);
      TEST_TIMESPEC_NOW_OR_AFTER (clockid, timeout);
    }

  /* Block a thread on two futexes and wake it on the second one; the
     return value reports the index of the woken futex.  */
  {
    pthread_t thr = xpthread_create (NULL, waiter_tf, NULL);

    while (__atomic_load_n (&waiter_result, __ATOMIC_ACQUIRE) == 0)
      futex_wake (&ftx2, INT_MAX, FUTEX_PRIVATE_FLAG);
    TEST_COMPARE (__atomic_load_n (&waiter_result, __ATOMIC_ACQUIRE), 1 + 1);

    xpthread_join (thr);
  }

  return 0;
}

#else /* !__USE_TIME_BITS64 */

static int
do_test (void)
{
  FAIL_UNSUPPORTED ("futex_waitv is only declared when time_t is a "
		    "64-bit type (build with -D_TIME_BITS=64)");
}

#endif

#include <support/test-driver.c>
