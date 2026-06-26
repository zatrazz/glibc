/* Malloc replacement module for tst-atfork5 (BZ 34321).
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

/* Minimal malloc interposer, applied to tst-atfork5 through LD_PRELOAD:
   every allocation takes ARENA_LOCK, and so does the fork prepare handler
   below (registered by the test program via pthread_atfork).  */

#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

/* Microseconds the prepare handler holds ARENA_LOCK.  */
#define WINDOW_US 4000

static pthread_mutex_t arena_lock = PTHREAD_MUTEX_INITIALIZER;
static void *(*real_malloc) (size_t);
static void *(*real_calloc) (size_t, size_t);
static void *(*real_realloc) (void *, size_t);
static void  (*real_free) (void *);

/* Number of allocations that went through the interposer; the test program
   uses it to check that this module was actually preloaded.  */
atomic_uint atfork5mod_malloc_count;

/* Small bump buffer to provide the few allocations that happen while dlsym
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

static void *
next_sym (const char *name)
{
  void *sym = dlsym (RTLD_NEXT, name);
  if (sym == NULL)
    abort ();
  return sym;
}

static void
init_real (void)
{
  if (real_malloc != NULL)
    return;
  in_dlsym = true;
  real_malloc = next_sym ("malloc");
  real_calloc = next_sym ("calloc");
  real_realloc = next_sym ("realloc");
  real_free = next_sym ("free");
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
  atomic_fetch_add (&atfork5mod_malloc_count, 1);
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
  atomic_fetch_add (&atfork5mod_malloc_count, 1);
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
  atomic_fetch_add (&atfork5mod_malloc_count, 1);
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
  pthread_mutex_lock (&arena_lock);
  real_free (p);
  pthread_mutex_unlock (&arena_lock);
}

/* Synchronizes the registering threads of the test program with the first
   prepare handler run, so that they only start registering once ARENA_LOCK
   is held by the prepare handler below.  Initialized by the test program.  */
pthread_barrier_t atfork5mod_window_barrier;
static atomic_int synced;

void
atfork5mod_prepare (void)
{
  pthread_mutex_lock (&arena_lock);
  /* Release the registering threads on the first fork only; on later forks
     they are no longer waiting on the barrier.  */
  if (!atomic_exchange (&synced, 1))
    pthread_barrier_wait (&atfork5mod_window_barrier);
  usleep (WINDOW_US);
}

void
atfork5mod_parent (void)
{
  pthread_mutex_unlock (&arena_lock);
}

void
atfork5mod_child (void)
{
  pthread_mutex_unlock (&arena_lock);
}

__attribute__ ((constructor))
static void
init (void)
{
  init_real ();
}
