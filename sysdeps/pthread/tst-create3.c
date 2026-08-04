/* Verify that concurrent dlopen of the same DSO is safe when ELF
   constructors run without dl_load_lock (BZ 15686).
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

/* Two invariants are checked for concurrent dlopen of the same DSO:

   1. The DSO's constructor runs exactly once, even if N threads race
      into _dl_open.

   2. No dlopen caller returns before the constructor has finished.
      A caller that beats the constructor would observe globals in
      their BSS-zeroed state -- a regression from the model where
      dl_load_lock serialized the whole _dl_open call.

   The DSO constructor sleeps ~200 ms to widen the race window, then
   publishes a "done" magic as its final write.  Each thread checks
   the magic immediately after dlopen returns; any thread observing
   the wrong value has raced ahead of the constructor.

   The ctor-runs-once count is read from a handle that is kept open
   across the check: every caller leaves its dlopen handle pinned in
   the handles[] array and main reads tst_create3mod_ctor_count via
   xdlsym on handles[0] before any dlclose.  Closing the handles
   first would unload the DSO once the last reference dropped, and a
   subsequent re-open would load a fresh instance whose counter
   starts at zero -- the count would then reflect only the
   single-threaded re-open and could not detect a constructor that
   ran more than once during the concurrent phase.  */

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <support/check.h>
#include <support/xdlfcn.h>
#include <support/xthread.h>

#include "tst-create3.h"

/* More threads than two so the race is exercised on multiple
   pair-wise combinations; small enough to keep the test cheap.  */
#define NTHREADS 8

static pthread_barrier_t g_start_barrier;
static void *handles[NTHREADS];

static void *
worker (void *arg)
{
  int idx = (int) (intptr_t) arg;

  /* Release all workers at once so they enter _dl_open
     near-simultaneously.  */
  xpthread_barrier_wait (&g_start_barrier);

  void *h = xdlopen ("tst-create3mod.so", RTLD_NOW);

  /* The "done" flag is the constructor's final write.  If dlopen
     returned before the constructor finished (the early return that
     _dl_init_wait prevents), this load will observe 0 instead of
     TST_CREATE3_MAGIC_DONE.  */
  _Atomic unsigned int *done = xdlsym (h, "tst_create3mod_done");
  unsigned int done_val = atomic_load_explicit (done, memory_order_acquire);
  if (done_val != TST_CREATE3_MAGIC_DONE)
    FAIL ("thread %d returned from dlopen before the constructor finished"
	  " (tst_create3mod_done=0x%x)", idx, done_val);

  /* Keep the handle open; main reads the ctor counter from it and
     dlcloses all handles afterwards.  */
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

  /* The DSO is still loaded (all handles open), so the counter
     reflects every constructor execution during the race above.  */
  _Atomic int *count = xdlsym (handles[0], "tst_create3mod_ctor_count");
  TEST_COMPARE (atomic_load_explicit (count, memory_order_acquire), 1);

  for (int i = 0; i < NTHREADS; ++i)
    xdlclose (handles[i]);

  return 0;
}

#include <support/test-driver.c>
