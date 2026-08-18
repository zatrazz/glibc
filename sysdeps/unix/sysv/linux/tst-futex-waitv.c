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
      { .addr = &ftx1, .expected = 0, .flags = FUTEX_FLAG_PRIVATE },
      { .addr = &ftx2, .expected = 0, .flags = FUTEX_FLAG_PRIVATE },
    };
  unsigned int index;
  int r;
  do
    r = futex_waitv (waiters, 2, CLOCK_MONOTONIC, NULL, &index);
  while (r == EINTR);
  TEST_COMPARE (r, 0);
  __atomic_store_n (&waiter_result, index + 1, __ATOMIC_RELEASE);
  return NULL;
}

static int
do_test (void)
{
  uint32_t word = 5;
  struct futex_waiter waiter =
    { .addr = &word, .expected = 4, .flags = FUTEX_FLAG_PRIVATE };

  /* Use a value mismatch to probe for kernel support (Linux 5.16).  */
  {
    int r = futex_waitv (&waiter, 1, CLOCK_MONOTONIC, NULL, NULL);
    if (r == ENOSYS)
      FAIL_UNSUPPORTED ("kernel does not support the futex_waitv syscall");
    TEST_COMPARE (r, EAGAIN);
  }

  /* Invalid number of waiters and invalid per-waiter flags.  */
  TEST_COMPARE (futex_waitv (&waiter, 0, CLOCK_MONOTONIC, NULL, NULL),
		EINVAL);
  TEST_COMPARE (futex_waitv (&waiter, FUTEX_WAITV_MAX + 1, CLOCK_MONOTONIC,
			     NULL, NULL), EINVAL);
  {
    struct futex_waiter bad =
      { .addr = &word, .expected = 5, .flags = ~FUTEX_FLAG_SHARED };
    TEST_COMPARE (futex_waitv (&bad, 1, CLOCK_MONOTONIC, NULL, NULL),
		  EINVAL);
  }

  /* Invalid clock.  */
  {
    struct timespec ts = make_timespec (0, 0);
    waiter.expected = 5;
    TEST_COMPARE (futex_waitv (&waiter, 1, CLOCK_PROCESS_CPUTIME_ID, &ts,
			       NULL), EINVAL);
  }

  /* Timeouts in the past, including negative tv_sec.  */
  {
    struct timespec ts = make_timespec (0, 0);
    TEST_COMPARE (futex_waitv (&waiter, 1, CLOCK_MONOTONIC, &ts, NULL),
		  ETIMEDOUT);
    ts = make_timespec (-1, 0);
    TEST_COMPARE (futex_waitv (&waiter, 1, CLOCK_REALTIME, &ts, NULL),
		  ETIMEDOUT);
  }

  /* An actual short wait on both supported clocks.  */
  for (int i = 0; i < 2; i++)
    {
      clockid_t clockid = i == 0 ? CLOCK_REALTIME : CLOCK_MONOTONIC;
      struct timespec timeout
	= timespec_add (xclock_now (clockid), make_timespec (0, 100000000));
      TEST_COMPARE (futex_waitv (&waiter, 1, clockid, &timeout, NULL),
		    ETIMEDOUT);
      TEST_TIMESPEC_NOW_OR_AFTER (clockid, timeout);
    }

  /* Block a thread on two futexes and wake it on the second one; the
     index of the woken futex is stored in *INDEX.  */
  {
    pthread_t thr = xpthread_create (NULL, waiter_tf, NULL);

    while (__atomic_load_n (&waiter_result, __ATOMIC_ACQUIRE) == 0)
      futex_wake (&ftx2, INT_MAX, FUTEX_FLAG_PRIVATE);
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
