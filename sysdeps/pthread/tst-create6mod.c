/* DSO for tst-create6: constructor spawning a thread while another
   thread waits for the constructor (BZ 15686).
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

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>

#include "tst-create6.h"

/* Number of times the constructor ran.  Must end up == 1.  */
_Atomic int tst_create6mod_ctor_count = 0;

/* Published as the constructor's final write; dlopen callers check
   it to prove they did not return before the constructor finished.  */
_Atomic unsigned int tst_create6mod_done = 0;

static void *
worker (void *arg)
{
  /* Merely existing is enough: creating this thread already required
     dl_load_tls_lock.  */
  return NULL;
}

static void __attribute__ ((constructor))
do_init (void)
{
  atomic_fetch_add_explicit (&tst_create6mod_ctor_count, 1,
			     memory_order_relaxed);

  /* Give a concurrent dlopen caller time to reach the constructor
     wait before we need dl_load_tls_lock below.  */
  struct timespec ts = { .tv_nsec = 200000000 };	/* 200 ms */
  nanosleep (&ts, NULL);

  /* pthread_create -> allocatestack -> _dl_allocate_tls_init takes
     dl_load_tls_lock.  If the concurrent dlopen caller waits for
     this constructor while holding dl_load_tls_lock (the bug this
     test covers), this call never returns and pthread_join below
     never completes: the test hangs and the test-driver timeout
     fires.  */
  pthread_t t;
  if (pthread_create (&t, NULL, worker, NULL) != 0)
    abort ();
  if (pthread_join (t, NULL) != 0)
    abort ();

  atomic_store_explicit (&tst_create6mod_done, TST_CREATE6_MAGIC_DONE,
			 memory_order_release);
}
