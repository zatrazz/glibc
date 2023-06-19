/* Check if thread local storate is restart on each SIGEV_THREAD trigger.
   Copyright (C) 2025 Free Software Foundation, Inc.
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

#include <array_length.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <support/check.h>
#include <support/xthread.h>

#include <stdio.h>

static pthread_barrier_t barrier;

static __thread int var1;
#define VAR2_LEN 32
static __thread char var2[] = { [0 ... VAR2_LEN] = 0xcc };

static const char var2_expected[] = { [0 ... VAR2_LEN] = 0xcc };

static void
on_timer (union sigval sv)
{
  TEST_COMPARE (var1 , 0);
  TEST_COMPARE_BLOB (var2, array_length (var2),
		     var2_expected, array_length (var2_expected));

  var1 = 1;
  memset (var2, 0x00, array_length (var2));

  xpthread_barrier_wait (&barrier);
}

#if 0
static void
on_timer2 (union sigval sv)
{
  TEST_COMPARE (var1 , 0);
  TEST_COMPARE_BLOB (var2, array_length (var2),
		     var2_expected, array_length (var2_expected));

  var1 = 1;
  memset (var2, 0x00, array_length (var2));

  xpthread_barrier_wait (&barrier);
}
#endif

#define NITERS 10

static timer_t
create_timer (void (*closure)(union sigval sv))
{
  timer_t timerid;
  struct sigevent ev =
    {
      .sigev_notify = SIGEV_THREAD,
      .sigev_notify_function = closure,
    };

  if (timer_create (CLOCK_REALTIME, &ev, &timerid) == -1)
    FAIL_EXIT1 ("timer_create: %m");

  return timerid;
}

static int
do_test (void)
{
  const struct itimerspec its1
      = { .it_value = { .tv_nsec = 10000000 /* 0.01s */ },
          .it_interval = { .tv_nsec = 10000000 /* 0.01s */ } };
  const struct itimerspec its2
      = { .it_value = { .tv_nsec = 10000000 /* 0.01s */ } };

  xpthread_barrier_init (&barrier, NULL, 3);

  timer_t timerid1 = create_timer (on_timer);
  if (timer_settime (timerid1, 0, &its1, NULL) == -1)
    FAIL_EXIT1 ("timer_settime: %m");

  for (int i = 0; i < NITERS; i++)
    {
      timer_t timerid2 = create_timer (on_timer);
      if (timer_settime (timerid2, 0, &its2, NULL) == -1)
	FAIL_EXIT1 ("timer_settime: %m");

      xpthread_barrier_wait (&barrier);

      if (timer_delete (timerid2) == -1)
	FAIL_EXIT1 ("time_delete: %m");
    }

  if (timer_delete (timerid1) == -1)
    FAIL_EXIT1 ("time_delete: %m");

  return 0;
}

#include <support/test-driver.c>
