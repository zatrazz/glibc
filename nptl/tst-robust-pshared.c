/* Test process-shared robust mutex support and lazy initialization (BZ 33225).
   Copyright (C) 2025 Free Software Foundation, Inc.
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

/* Verify that pthread_mutex_init returns ENOTSUP for a process-shared robust
   mutex when the set_robust_list syscall is not available (e.g., qemu-user),
   and that the lazy robust list initialization works correctly when the
   syscall is available.  */

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <support/check.h>
#include <support/test-driver.h>
#include <support/xthread.h>

/* Lock the mutex and exit without unlocking to trigger EOWNERDEAD.  */
static void *
owner_thread (void *arg)
{
  pthread_mutex_t *mutex = arg;
  TEST_COMPARE (pthread_mutex_lock (mutex), 0);
  /* Thread exits here without unlocking.  The kernel walks the robust
     list registered via set_robust_list and marks the mutex owner-dead.
     This verifies that lazy robust_list_setup correctly set futex_offset
     and called set_robust_list before the first lock.  */
  return NULL;
}

static int
do_test (void)
{
  bool robust_support = support_process_shared_robust_mutex ();
  if (test_verbose)
    printf ("info: process-shared robust mutex support: %d\n", robust_support);

  pthread_mutexattr_t attr;
  TEST_COMPARE (pthread_mutexattr_init (&attr), 0);
  TEST_COMPARE (pthread_mutexattr_setpshared (&attr, PTHREAD_PROCESS_SHARED),
		0);
  TEST_COMPARE (pthread_mutexattr_setrobust (&attr, PTHREAD_MUTEX_ROBUST), 0);

  pthread_mutex_t mutex;
  int ret = pthread_mutex_init (&mutex, &attr);
  TEST_COMPARE (pthread_mutexattr_destroy (&attr), 0);

  if (!robust_support)
    {
      /* When set_robust_list is unavailable, pshared+robust mutex init
	 must fail with ENOTSUP rather than silently succeeding.  */
      TEST_COMPARE (ret, ENOTSUP);
      return 0;
    }

  TEST_COMPARE (ret, 0);

  /* Have a thread lock the mutex and exit without unlocking.
     This exercises the lazy robust_list_setup path in
     pthread_mutex_lock: futex_offset must be set before set_robust_list
     is called so the kernel can correctly compute the lock address on
     thread death.  */
  pthread_t thread;
  TEST_COMPARE (pthread_create (&thread, NULL, owner_thread, &mutex), 0);
  TEST_COMPARE (pthread_join (thread, NULL), 0);

  TEST_COMPARE (pthread_mutex_lock (&mutex), EOWNERDEAD);
  TEST_COMPARE (pthread_mutex_consistent (&mutex), 0);
  TEST_COMPARE (pthread_mutex_unlock (&mutex), 0);
  TEST_COMPARE (pthread_mutex_destroy (&mutex), 0);

  return 0;
}

#include <support/test-driver.c>
