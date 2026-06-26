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
   and waits for the atfork lock.  */

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

#define WINDOW_US     4000    /* useconds_t prepare handler holds ARENA_LOCK.  */
#define NREG          3       /* Number of 'tf' threads.  */
#define FORK_TARGET   20      /* Forks to complete when there is no deadlock.  */
#define REG_CAP       40000U  /* Upper bound on total handler registrations.  */

/* Minimal malloc interposer: every allocation takes ARENA_LOCK, and so does
   the fork prepare handler below.  */
static pthread_mutex_t arena_lock = PTHREAD_MUTEX_INITIALIZER;
static void *(*real_malloc) (size_t);
static void *(*real_calloc) (size_t, size_t);
static void *(*real_realloc) (void *, size_t);
static void  (*real_free) (void *);

/* Small bump buffer ti provide the few allocations that happen while dlsym
   itself runs.  */
static __thread bool in_dlsym;
static char bootbuf[1 << 20];
static size_t bootoff;

static int
is_boot (void *p)
{
  return (char *) p >= bootbuf && (char *) p < bootbuf + sizeof bootbuf;
}

static void *
boot_alloc (size_t n)
{
  size_t o = (bootoff + 15) & ~(size_t) 15;
  bootoff = o + n;
  return bootbuf + o;
}

static void
init_real (void)
{
  if (real_malloc != NULL)
    return;
  in_dlsym = true;
  real_malloc = xdlsym (RTLD_NEXT, "malloc");
  real_calloc = xdlsym (RTLD_NEXT, "calloc");
  real_realloc = xdlsym (RTLD_NEXT, "realloc");
  real_free = xdlsym (RTLD_NEXT, "free");
  in_dlsym = false;
}

void *
malloc (size_t n)
{
  if (real_malloc == NULL)
    {
      if (in_dlsym)
	return boot_alloc (n);
      init_real ();
    }
  pthread_mutex_lock (&arena_lock);
  void *p = real_malloc (n);
  pthread_mutex_unlock (&arena_lock);
  return p;
}

void *
calloc (size_t a, size_t b)
{
  if (real_malloc == NULL)
    {
      if (in_dlsym)
	return boot_alloc (a * b);
      init_real ();
    }
  pthread_mutex_lock (&arena_lock);
  void *p = real_calloc (a, b);
  pthread_mutex_unlock (&arena_lock);
  return p;
}

void *
realloc (void *old, size_t n)
{
  if (real_malloc == NULL)
    init_real ();
  pthread_mutex_lock (&arena_lock);
  void *p = real_realloc (old, n);
  pthread_mutex_unlock (&arena_lock);
  return p;
}

void
free (void *p)
{
  if (is_boot (p))
    return;
  if (real_free == NULL)
    init_real ();
  xpthread_mutex_lock (&arena_lock);
  real_free (p);
  xpthread_mutex_unlock (&arena_lock);
}

/* Synchronizes the NREG tf threads with the first prepare handler run, so
   that they only start registering once ARENA_LOCK is held by the prepare
   handler below.  */
static pthread_barrier_t window_barrier;
static atomic_int synced;

static void
prepare (void)
{
  xpthread_mutex_lock (&arena_lock);
  /* Release the tf threads on the first fork only; on later forks they are
     no longer waiting on the barrier.  */
  if (!atomic_exchange (&synced, 1))
    xpthread_barrier_wait (&window_barrier);
  usleep (WINDOW_US);
}

static void
parent (void)
{
  xpthread_mutex_unlock (&arena_lock);
}

static void
child (void)
{
  xpthread_mutex_unlock (&arena_lock);
}

__attribute__ ((constructor))
static void
init (void)
{
  init_real ();
}

static atomic_int running = 1;
static atomic_int forks_done;
static atomic_uint reg_count;

static void *
tf (void *closure)
{
  /* Start registering only once a prepare window is open (ARENA_LOCK held),
     so the first handler-array growth happens under that lock.  The total
     number of registrations is bounded so that the handler list stays small
     and, on a fixed library, the fork handlers run quickly.  */
  xpthread_barrier_wait (&window_barrier);
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
  xpthread_barrier_init (&window_barrier, NULL, NREG + 1);
  TEST_COMPARE (pthread_atfork (prepare, parent, child), 0);

  pthread_t reg[NREG];
  for (int i = 0; i < NREG; i++)
    reg[i] = xpthread_create (NULL, tf, NULL);
  pthread_t fork_tid = xpthread_create (NULL, forker, NULL);

  xpthread_join (fork_tid);
  atomic_store (&running, 0);
  for (int i = 0; i < NREG; i++)
    xpthread_join (reg[i]);

  xpthread_barrier_destroy (&window_barrier);
  return 0;
}

#define TIMEOUT 8
#include <support/test-driver.c>
