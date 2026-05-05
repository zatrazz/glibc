/* Check that process-shared robust mutex creation follows kernel
   support (BZ #33225).
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
#include <pthread.h>
#include <stdbool.h>

#include <support/check.h>
#include <support/xthread.h>

/* Lock the mutex and exit without unlocking it, so that the owner-died
   notification has to be delivered to the next locker.  */
static void *
owner_thread (void *arg)
{
  pthread_mutex_t *mutex = arg;
  TEST_COMPARE (pthread_mutex_lock (mutex), 0);
  return NULL;
}

static int
do_test (void)
{
  bool robust_support = support_process_shared_robust_mutex ();

  for (int pshared = 0; pshared < 2; pshared++)
    for (int robust = 0; robust < 2; robust++)
      {
        pthread_mutexattr_t attr;
        xpthread_mutexattr_init (&attr);
        if (pshared)
          xpthread_mutexattr_setpshared (&attr, PTHREAD_PROCESS_SHARED);
        if (robust)
          xpthread_mutexattr_setrobust (&attr, PTHREAD_MUTEX_ROBUST);

        /* Only process-shared robust mutexes require the kernel to walk the
	   robust list on process exit, robust mutexes private to the process
	   are handled by pthread_create itself.  Non-robust mutexes do not
	   need the robust list at all.  */
        int expected = pshared && robust && !robust_support ? ENOTSUP : 0;

        pthread_mutex_t mtx;
        TEST_COMPARE (pthread_mutex_init (&mtx, &attr), expected);
        if (expected == 0)
          {
            TEST_COMPARE (pthread_mutex_lock (&mtx), 0);
            TEST_COMPARE (pthread_mutex_unlock (&mtx), 0);
            xpthread_mutex_destroy (&mtx);
          }

        xpthread_mutexattr_destroy (&attr);
      }

  /* Have a thread lock a robust mutex and exit without unlocking it.  This
     exercises the deferred robust_list_setup path in pthread_mutex_lock.  */
  {
    pthread_mutexattr_t attr;
    xpthread_mutexattr_init (&attr);
    xpthread_mutexattr_setrobust (&attr, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_t mutex;
    TEST_COMPARE (pthread_mutex_init (&mutex, &attr), 0);
    xpthread_mutexattr_destroy (&attr);

    xpthread_join (xpthread_create (NULL, owner_thread, &mutex));

    TEST_COMPARE (pthread_mutex_lock (&mutex), EOWNERDEAD);
    TEST_COMPARE (pthread_mutex_consistent (&mutex), 0);
    TEST_COMPARE (pthread_mutex_unlock (&mutex), 0);
    TEST_COMPARE (pthread_mutex_destroy (&mutex), 0);
  }

  return 0;
}

#include <support/test-driver.c>
