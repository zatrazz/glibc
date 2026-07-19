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

#include <stdlib.h>
#include <dso_handle.h>
#include <support/check.h>
#include <support/xthread.h>

int tst_create2mod_dtor_done;

static void
dtor (void *obj)
{
  *(int *) obj = 1;
}

/* The module TLS access mirrors the real-world trigger (a C++ thread_local
   or Rust thread_local! first access), exercising __tls_get_addr from the
   spawned thread as well.  */
static __thread int tls_obj;

static void *
worker (void *closure)
{
  tls_obj = 1;
  TEST_COMPARE (__cxa_thread_atexit_impl (dtor, &tst_create2mod_dtor_done,
					  __dso_handle), 0);
  return NULL;
}

static void __attribute__ ((constructor))
do_init (void)
{
  xpthread_join (xpthread_create (NULL, worker, NULL));
}
