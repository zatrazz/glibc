/* Test that atfork registration does not deadlock against a malloc
   replacement (BZ 34321).
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

/* Check if a malloc replacement that takes its own lock both in malloc and
   in a registered fork prepare handler does not deadlock.  One thread holds
   the atfork lock and waits for the allocator lock inside __register_atfork,
   while the forking thread holds the allocator lock in the prepare handler
   and waits for the atfork lock.

   The malloc replacement is provided by tst-atfork5mod.so through
   LD_PRELOAD.  */

#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <support/check.h>
#include <support/xthread.h>
#include <support/xunistd.h>
#include <support/xdlfcn.h>

#define NREG          3       /* Number of 'tf' threads.  */
#define FORK_TARGET   20      /* Forks to complete when there is no deadlock.  */
#define REG_CAP       40000U  /* Upper bound on total handler registrations.  */

/* Synchronizes the NREG tf threads with the first prepare handler run, so
   that they only start registering once the allocator lock is held by the
   preloaded module's prepare handler.  */
static pthread_barrier_t *window_barrier;

static atomic_int running = 1;
static atomic_int forks_done;
static atomic_uint reg_count;

static void *
tf (void *closure)
{
  /* Start registering only once a prepare window is open (the module's
     allocator lock held), so the first handler-list growth happens under
     that lock.  The total number of registrations is bounded so that the
     handler list stays small and, on a fixed library, the fork handlers
     run quickly.  */
  xpthread_barrier_wait (window_barrier);
  while (atomic_load (&running)
	 && atomic_fetch_add (&reg_count, 1) < REG_CAP)
    pthread_atfork (NULL, NULL, NULL);
  return NULL;
}

static void *
forker (void *closure)
{
  while (atomic_load (&running))
    {
      pid_t pid = xfork ();
      if (pid == 0)
	_exit (0);
      xwaitpid (pid, NULL, 0);
      if (atomic_fetch_add (&forks_done, 1) + 1 >= FORK_TARGET)
	break;
    }
  return NULL;
}

static int
do_test (void)
{
  /* Check if tst-atfork5mod.so was preloaded.  */
  void (*mod_prepare) (void) = xdlsym (RTLD_DEFAULT, "atfork5mod_prepare");
  void (*mod_parent) (void) = xdlsym (RTLD_DEFAULT, "atfork5mod_parent");
  void (*mod_child) (void) = xdlsym (RTLD_DEFAULT, "atfork5mod_child");
  atomic_uint *malloc_count = xdlsym (RTLD_DEFAULT,
				      "atfork5mod_malloc_count");
  window_barrier = xdlsym (RTLD_DEFAULT, "atfork5mod_window_barrier");

  xpthread_barrier_init (window_barrier, NULL, NREG + 1);
  TEST_COMPARE (pthread_atfork (mod_prepare, mod_parent, mod_child), 0);

  pthread_t reg[NREG];
  for (int i = 0; i < NREG; i++)
    reg[i] = xpthread_create (NULL, tf, NULL);
  pthread_t fork_tid = xpthread_create (NULL, forker, NULL);

  xpthread_join (fork_tid);
  atomic_store (&running, 0);
  for (int i = 0; i < NREG; i++)
    xpthread_join (reg[i]);

  TEST_VERIFY (atomic_load (malloc_count) > 0);

  xpthread_barrier_destroy (window_barrier);
  return 0;
}

#define TIMEOUT 8
#include <support/test-driver.c>
