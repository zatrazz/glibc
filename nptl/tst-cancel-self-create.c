/* Check pthread_create after a pthread_cancel of the calling thread.
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

/* A pthread_cancel of the calling thread in a single-threaded process
   should not prevent the first pthread_create from switching the
   process to multi-threaded mode: installing the SIGSETXID handler used
   by setuid and related functions, and clearing
   __libc_single_threaded.  */

#include <pthread.h>
#include <sys/single_threaded.h>
#include <unistd.h>
#include <support/check.h>
#include <support/xthread.h>

static pthread_barrier_t barrier;

static void *
thread_func (void *closure)
{
  /* Keep the thread alive until the main thread has called setuid.  */
  xpthread_barrier_wait (&barrier);
  return NULL;
}

static int
do_test (void)
{
  /* Disable cancellation so the request stays pending.  */
  TEST_COMPARE (pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL), 0);
  TEST_COMPARE (pthread_cancel (pthread_self ()), 0);

  xpthread_barrier_init (&barrier, NULL, 2);
  pthread_t thr = xpthread_create (NULL, thread_func, NULL);

  TEST_VERIFY (!__libc_single_threaded);

  /* setuid signals the other thread with SIGSETXID, which terminates
     the process if the handler is not installed.  */
  if (setuid (getuid ()) != 0)
    FAIL_EXIT1 ("setuid: %m");

  xpthread_barrier_wait (&barrier);
  xpthread_join (thr);
  xpthread_barrier_destroy (&barrier);

  return 0;
}

#include <support/test-driver.c>
