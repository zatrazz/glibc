/* Verify that waiting for another thread's in-progress constructor
   does not block thread creation from that constructor (BZ 15686).
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

/* The BZ 15686 fix makes a concurrent dlopen of the same DSO wait
   until the constructor completes.  That wait must be performed
   without holding dl_load_tls_lock: if the constructor spawns a
   thread, thread creation needs dl_load_tls_lock
   (_dl_allocate_tls_init), which would produce a deadlock cycle:

     waiter:  holds dl_load_tls_lock, waits for the constructor
     loader:  runs ctor, waits for pthread_create -> dl_load_tls_lock

   This test combines the two ingredients that tst-create2 and
   tst-create3 exercise separately: a constructor that spawns and
   joins a thread (tst-create6mod.c), and two threads concurrently
   dlopening the same DSO.  The constructor sleeps 200 ms before
   pthread_create so the second caller reliably reaches the wait
   first.

   If the wait holds dl_load_tls_lock, the test deadlocks and the
   test-driver timeout fires; otherwise both dlopen calls return only
   after the constructor published its done magic, and the
   constructor ran exactly once.  */

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <support/check.h>
#include <support/xdlfcn.h>
#include <support/xthread.h>

#include "tst-create6.h"

/* Two threads: one becomes the loader running the constructor, the
   other becomes the waiter.  More waiters would not change the
   mechanism.  */
#define NTHREADS 2

static pthread_barrier_t g_start_barrier;
static void *handles[NTHREADS];

static void *
worker (void *arg)
{
  int idx = (int) (intptr_t) arg;

  /* Release both workers at once so they enter _dl_open
     near-simultaneously.  */
  xpthread_barrier_wait (&g_start_barrier);

  void *h = xdlopen ("tst-create6mod.so", RTLD_NOW);

  /* If dlopen returned before the constructor finished, the done
     magic is not yet visible.  */
  _Atomic unsigned int *done = xdlsym (h, "tst_create6mod_done");
  unsigned int done_val = atomic_load_explicit (done, memory_order_acquire);
  if (done_val != TST_CREATE6_MAGIC_DONE)
    FAIL ("thread %d returned from dlopen before the constructor finished"
	  " (tst_create6mod_done=0x%x)", idx, done_val);

  /* Keep the handle open so the ctor counter below reflects the
     concurrent phase.  */
  handles[idx] = h;
  return NULL;
}

static int
do_test (void)
{
  pthread_t threads[NTHREADS];

  xpthread_barrier_init (&g_start_barrier, NULL, NTHREADS);

  for (int i = 0; i < NTHREADS; ++i)
    threads[i] = xpthread_create (0, worker, (void *) (intptr_t) i);

  for (int i = 0; i < NTHREADS; ++i)
    xpthread_join (threads[i]);

  /* Both dlopen calls returned; the DSO is still loaded via the
     pinned handles.  The constructor must have run exactly once.  */
  _Atomic int *count = xdlsym (handles[0], "tst_create6mod_ctor_count");
  TEST_COMPARE (atomic_load_explicit (count, memory_order_acquire), 1);

  for (int i = 0; i < NTHREADS; ++i)
    xdlclose (handles[i]);

  return 0;
}

#include <support/test-driver.c>
