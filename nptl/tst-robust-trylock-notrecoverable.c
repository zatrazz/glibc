/* Test that pthread_mutex_trylock does not leave a robust mutex locked
   when it fails with ENOTRECOVERABLE (bug 27458).
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

/* Once a robust mutex has become not recoverable (the owner died and
   the next owner unlocked it without calling pthread_mutex_consistent),
   POSIX requires every subsequent pthread_mutex_lock and
   pthread_mutex_trylock call to fail with ENOTRECOVERABLE without
   acquiring the mutex.  pthread_mutex_trylock used to acquire the
   mutex and then return ENOTRECOVERABLE without releasing it, so that
   the next trylock returned EBUSY and the next lock blocked forever.  */

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <support/check.h>
#include <support/timespec.h>
#include <support/xthread.h>
#include <support/xtime.h>

static pthread_mutex_t mutex;

static void *
lock_and_exit (void *arg)
{
  TEST_COMPARE (pthread_mutex_lock (&mutex), 0);
  /* Exit while holding the mutex, so that the next owner sees
     EOWNERDEAD.  */
  return NULL;
}

static void *
trylock_in_thread (void *arg)
{
  int *result = arg;
  *result = pthread_mutex_trylock (&mutex);
  return NULL;
}

static void
run_test (bool pshared, bool pi)
{
  printf ("info: pshared=%d pi=%d\n", pshared, pi);

  pthread_mutexattr_t attr;
  xpthread_mutexattr_init (&attr);
  xpthread_mutexattr_setrobust (&attr, PTHREAD_MUTEX_ROBUST);
  if (pshared)
    xpthread_mutexattr_setpshared (&attr, PTHREAD_PROCESS_SHARED);
  if (pi)
    xpthread_mutexattr_setprotocol (&attr, PTHREAD_PRIO_INHERIT);
  xpthread_mutex_init (&mutex, &attr);
  xpthread_mutexattr_destroy (&attr);

  /* Make the mutex not recoverable.  */
  xpthread_join (xpthread_create (NULL, lock_and_exit, NULL));
  TEST_COMPARE (pthread_mutex_lock (&mutex), EOWNERDEAD);
  TEST_COMPARE (pthread_mutex_unlock (&mutex), 0);

  /* From now on, no lock operation may succeed or block.  */
  TEST_COMPARE (pthread_mutex_lock (&mutex), ENOTRECOVERABLE);
  TEST_COMPARE (pthread_mutex_trylock (&mutex), ENOTRECOVERABLE);

  /* The previous trylock must not have acquired the mutex: a second
     trylock has to fail with ENOTRECOVERABLE again, not EBUSY, in the
     same thread and in another one.  */
  TEST_COMPARE (pthread_mutex_trylock (&mutex), ENOTRECOVERABLE);
  int result = -1;
  xpthread_join (xpthread_create (NULL, trylock_in_thread, &result));
  TEST_COMPARE (result, ENOTRECOVERABLE);

  /* Likewise, a blocking lock has to fail immediately.  Use a timed
     lock so that the test does not hang if the mutex was left
     locked.  */
  struct timespec timeout = timespec_add (xclock_now (CLOCK_REALTIME),
					  make_timespec (1, 0));
  TEST_COMPARE (pthread_mutex_timedlock (&mutex, &timeout), ENOTRECOVERABLE);

  /* The only remaining permitted operation.  */
  TEST_COMPARE (pthread_mutex_destroy (&mutex), 0);
}

static int
do_test (void)
{
  run_test (false, false);
  run_test (true, false);
  run_test (false, true);
  run_test (true, true);
  return 0;
}

#include <support/test-driver.c>
