/* DSO for tst-create3: concurrent dlopen constructor-once test (BZ 15686).
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

#include <stdatomic.h>
#include <time.h>

#include "tst-create3.h"

/* How long (nanoseconds) the constructor sleeps to simulate slow
   initialization.  Large enough that concurrent dlopen callers are
   very likely to enter _dl_open while the constructor is still
   running.  */
#define TST_CREATE3_CTOR_SLEEP_NS 200000000	/* 200 ms */

/* Counter incremented by the constructor.  Must be exactly 1 after
   concurrent dlopen -- catches the case where two threads both claim
   the constructor execution and run it in parallel.  */
_Atomic int tst_create3mod_ctor_count = 0;

/* Visibility marker.  Set as the final write of the constructor with
   release ordering.  Concurrent dlopen callers must observe
   TST_CREATE3_MAGIC_DONE here after their dlopen returns.  */
_Atomic unsigned int tst_create3mod_done = 0;

static void
sleep_ns (long int ns)
{
  struct timespec ts =
    {
      .tv_sec = ns / 1000000000L,
      .tv_nsec = ns % 1000000000L
    };
  nanosleep (&ts, NULL);
}

static void __attribute__ ((constructor))
do_init (void)
{
  /* Record that we ran.  Two threads both claiming the constructor
     execution would increment this more than once.  */
  atomic_fetch_add_explicit (&tst_create3mod_ctor_count, 1,
			     memory_order_relaxed);

  /* Slow the constructor down to widen the window in which a buggy
     call_init would let a concurrent caller return from dlopen.  */
  sleep_ns (TST_CREATE3_CTOR_SLEEP_NS);

  /* Publish "constructor finished" as the last write with release
     ordering, so that callers observing TST_CREATE3_MAGIC_DONE also
     observe every earlier write the constructor made.  */
  atomic_store_explicit (&tst_create3mod_done, TST_CREATE3_MAGIC_DONE,
			 memory_order_release);
}
