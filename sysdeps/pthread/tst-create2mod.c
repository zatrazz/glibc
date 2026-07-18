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

#include <pthread.h>
#include <stdlib.h>
#include <dso_handle.h>

/* Registered as the TLS destructor for the worker thread; called with
   a pointer to OBJ below when the worker thread exits.  */
static void
dtor (void *obj)
{
  *(int *) obj = 1;
}

static __thread int obj;

/* First TLS destructor registration in the worker thread; with the
   pre-BZ-15686 implementation of __cxa_thread_atexit_impl this blocks
   on dl_load_lock, which is held by the thread running the dlopen that
   executes this module's constructor.  */
static void *
worker (void *arg)
{
  extern int __cxa_thread_atexit_impl (void (*) (void *), void *, void *);
  if (__cxa_thread_atexit_impl (dtor, &obj, __dso_handle) != 0)
    abort ();
  return NULL;
}

static void __attribute__ ((constructor))
do_init (void)
{
  pthread_t t;
  if (pthread_create (&t, NULL, worker, NULL) != 0)
    abort ();
  /* Blocks until the worker has completed its __cxa_thread_atexit_impl
     call; under the old locking model that call deadlocks against the
     dl_load_lock held by the dlopen caller running this constructor.  */
  if (pthread_join (t, NULL) != 0)
    abort ();
}
