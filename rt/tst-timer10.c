/* Check that timer_getoverrun accumulates SIGEV_THREAD overruns across
   multiple notifications.
   Copyright (C) 2026 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, see <https://www.gnu.org/licenses/>.  */


/* A periodic timer whose notification function runs longer than the interval
   overruns on every firing.  Because the helper serves one notification at a
   time and the missed expirations are delivered promptly (SIGTIMER unblocked)
   rather than left pending, they are tracked in userspace.  The count must be
   cumulative and overruns from earlier notifications must not be lost when
   a new notification starts.  */

#include <signal.h>
#include <stdatomic.h>
#include <time.h>

#include <support/check.h>

enum { interval_nsec = 10000000 };  /* 0.01s */
enum { run_intervals = 4 };
enum { notifications = 6 };

static atomic_int reached_final;
static atomic_int keep_spinning = 1;
static atomic_int overrun_cumulative;

static timer_t timerid;

static void
busy_wait (long int nsec)
{
  struct timespec start, now;
  clock_gettime (CLOCK_MONOTONIC, &start);
  do
    clock_gettime (CLOCK_MONOTONIC, &now);
  while ((now.tv_sec - start.tv_sec) * NSEC_PER_SEC
	 + (now.tv_nsec - start.tv_nsec) < nsec);
}

static void
on_timer (union sigval sv)
{
  static int n;

  if (++n < notifications)
    /* Run past several interval boundaries so this notification overruns.  */
    busy_wait ((long int) run_intervals * interval_nsec);
  else if (n == notifications)
    {
      /* Cumulative overruns from the prior notifications.  */
      atomic_store_explicit (&overrun_cumulative, timer_getoverrun (timerid),
			     memory_order_relaxed);
      atomic_store_explicit (&reached_final, 1, memory_order_release);
      while (atomic_load_explicit (&keep_spinning, memory_order_acquire))
	;
    }
}

static int
do_test (void)
{
  struct sigevent ev =
    {
      .sigev_notify = SIGEV_THREAD,
      .sigev_notify_function = on_timer,
    };
  TEST_COMPARE (timer_create (CLOCK_MONOTONIC, &ev, &timerid), 0);

  struct itimerspec its =
    { .it_value    = { .tv_nsec = interval_nsec },
      .it_interval = { .tv_nsec = interval_nsec } };
  TEST_COMPARE (timer_settime (timerid, 0, &its, NULL), 0);

  while (atomic_load_explicit (&reached_final, memory_order_acquire) == 0)
    ;

  /* Each of the (notifications - 1) prior notifications missed about
     (run_intervals - 1) expirations, so the cumulative count is several
     times what one notification alone contributes.  */
  int overrun = atomic_load_explicit (&overrun_cumulative,
				      memory_order_relaxed);
  TEST_VERIFY (overrun >= notifications);
  printf ("debug: overrun = %d\n", overrun);

  struct itimerspec its_stop = { 0 };
  TEST_COMPARE (timer_settime (timerid, 0, &its_stop, NULL), 0);
  atomic_store_explicit (&keep_spinning, 0, memory_order_release);
  TEST_COMPARE (timer_delete (timerid), 0);

  return 0;
}

#define TIMEOUT 15
#include <support/test-driver.c>
