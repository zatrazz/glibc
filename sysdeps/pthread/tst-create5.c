/* Verify that dlopen(NULL) from a worker thread does not deadlock
   after the main executable has been initialized (BZ 15686).
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

/* The main executable's constructors are not run by call_init in
   elf/dl-init.c; they are handled by the startup code.  With the
   per-map constructor states added for BZ 15686, call_init must
   nevertheless move the main executable's map to the lm_init_done
   state.  Otherwise a later multi-threaded dlopen (NULL) /
   __RTLD_OPENEXEC takes the already-open path in _dl_open, finds the
   map in a non-final state, and waits forever for a constructor
   execution that will never happen.

   This test spawns a worker thread that calls dlopen (NULL,
   RTLD_NOW).  If the bug is present, the call deadlocks and the
   test-driver timeout surfaces the failure.  */

#include <stdio.h>
#include <support/xdlfcn.h>
#include <support/xthread.h>

static void *
worker (void *arg)
{
  printf ("info: worker dlopen (NULL)\n");
  void *h = xdlopen (NULL, RTLD_NOW);
  printf ("info: worker dlopen (NULL) done\n");
  xdlclose (h);
  return NULL;
}

static int
do_test (void)
{
  pthread_t t = xpthread_create (0, worker, NULL);
  xpthread_join (t);

  return 0;
}

#include <support/test-driver.c>
