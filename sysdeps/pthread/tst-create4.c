/* Verify that a long-running dlopen constructor does not block a
   concurrent dlopen of an unrelated library (BZ 15686).

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

/* Thread A calls dlopen on a library whose constructor blocks on a
   barrier.  Once the constructor is executing (i.e. dl_load_lock has
   been released for the constructor execution), thread B calls
   dlopen on an unrelated library.

   When dl_load_lock was held across the entire dlopen call,
   including constructor execution, thread B's dlopen would block on
   dl_load_lock forever.  With the BZ 15686 fix, dl_load_lock is
   released for the duration of the constructor, so thread B's dlopen
   proceeds in parallel.

   The test is deterministic (no probabilistic race): we spin until
   the constructor confirms it has started, then launch thread B.  */

#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>
#include <stdio.h>
#include <support/xdlfcn.h>
#include <support/xthread.h>

#include "tst-create4.h"

/* Exported for the DSOs via -Wl,-export-dynamic (LDFLAGS-tst-create4).  */
pthread_barrier_t tst_create4_ctor_barrier;
atomic_int tst_create4_ctor_running = 0;

static void *
worker_a (void *unused)
{
  void *h = xdlopen ("tst-create4mod-a.so", RTLD_NOW);
  xdlclose (h);
  return NULL;
}

static void *
worker_b (void *unused)
{
  void *h = xdlopen ("tst-create4mod-b.so", RTLD_NOW);
  xdlclose (h);
  return NULL;
}

static int
do_test (void)
{
  xpthread_barrier_init (&tst_create4_ctor_barrier, NULL, 2);

  /* Start thread A.  It enters the constructor of tst-create4mod-a,
     which sets ctor_running = 1 and then blocks on the barrier.  */
  pthread_t ta = xpthread_create (0, worker_a, NULL);

  /* Spin until the constructor has definitely started.  */
  while (atomic_load_explicit (&tst_create4_ctor_running,
                               memory_order_acquire) == 0)
    sched_yield ();

  /* Now launch thread B.  If dl_load_lock is released during
     constructors (BZ 15686 fix), thread B's dlopen will succeed.
     If the lock is still held by thread A, thread B blocks on
     dl_load_lock and the test will time out.  */
  pthread_t tb = xpthread_create (0, worker_b, NULL);

  /* Wait for thread B to finish.  */
  xpthread_join (tb);

  printf ("info: concurrent dlopen of unrelated library succeeded\n");

  /* Signal the barrier so thread A's constructor can return.  */
  xpthread_barrier_wait (&tst_create4_ctor_barrier);
  xpthread_join (ta);

  return 0;
}

#include <support/test-driver.c>
