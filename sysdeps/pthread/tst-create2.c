/* Verify that a thread spawned by a dlopen constructor can register a
   TLS destructor without deadlocking (BZ 15686).
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

/* Reproducer for one instance of the deadlock class described in
   BZ 15686:

   thread 1: dlopen -> ctor -> pthread_create (worker) -> pthread_join
   thread 2 (worker): __cxa_thread_atexit_impl -> lock (dl_load_lock)

   dl_load_lock is held by thread 1 across the constructor execution,
   so if __cxa_thread_atexit_impl acquires it the worker thread blocks
   forever and pthread_join in the constructor never returns.  The
   test-driver timeout then reports the deadlock as a failure.  */

#include <stdio.h>
#include <support/xdlfcn.h>

static int
do_test (void)
{
  dprintf (1, "main: dlopen tst-create2mod.so\n");
  void *h = xdlopen ("tst-create2mod.so", RTLD_NOW);
  dprintf (1, "main: dlopen done\n");
  xdlclose (h);
  dprintf (1, "main: dlclose done\n");
  return 0;
}

#include <support/test-driver.c>
