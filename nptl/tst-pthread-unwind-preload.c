/* Check that the first pthread_create loads the unwinder (BZ 34689).
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

#include <dlfcn.h>
#include <gnu/lib-names.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/resource.h>
#include <unistd.h>
#include <support/check.h>
#include <support/namespace.h>
#include <support/xdlfcn.h>
#include <support/xthread.h>
#include <support/xunistd.h>

#ifndef HAVE_CC_WITH_LIBUNWIND
# define UNWIND_SONAME LIBGCC_S_SO
#else
# define UNWIND_SONAME LIBUNWIND_SO
#endif

/* Return true if the unwinder is already loaded, without loading it.  */
static bool
unwinder_loaded (void)
{
  void *handle = dlopen (UNWIND_SONAME, RTLD_LAZY | RTLD_NOLOAD);
  if (handle == NULL)
    return false;
  xdlclose (handle);
  return true;
}

static struct rlimit saved_nofile;

/* Make any further file descriptor allocation, and thus dlopen, fail
   with EMFILE.  */
static void
restrict_nofile (void)
{
  if (getrlimit (RLIMIT_NOFILE, &saved_nofile) != 0)
    FAIL_EXIT1 ("getrlimit (RLIMIT_NOFILE): %m");
  struct rlimit rl = saved_nofile;
  rl.rlim_cur = 0;
  if (setrlimit (RLIMIT_NOFILE, &rl) != 0)
    FAIL_EXIT1 ("setrlimit (RLIMIT_NOFILE): %m");
}

static void
restore_nofile (void)
{
  if (setrlimit (RLIMIT_NOFILE, &saved_nofile) != 0)
    FAIL_EXIT1 ("setrlimit (RLIMIT_NOFILE): %m");
}

static void *
thread_return (void *closure)
{
  return closure;
}

static void *
thread_exit (void *closure)
{
  pthread_exit (closure);
}

static void *
thread_block (void *closure)
{
  /* pause is a cancellation point.  */
  while (true)
    pause ();
  return NULL;
}

/* The first pthread_create loads the unwinder, so pthread_exit and
   pthread_cancel keep working after the process can no longer open
   files.  Without the early load both calls would abort the process
   with "libgcc_s.so.1 must be installed for ... to work".  */
static void
test_preload (void *closure)
{
  TEST_VERIFY (!unwinder_loaded ());
  xpthread_join (xpthread_create (NULL, thread_return, NULL));
  TEST_VERIFY (unwinder_loaded ());

  restrict_nofile ();

  void *r = xpthread_join (xpthread_create (NULL, thread_exit,
					    (void *) (uintptr_t) 42));
  TEST_VERIFY (r == (void *) (uintptr_t) 42);

  pthread_t thr = xpthread_create (NULL, thread_block, NULL);
  xpthread_cancel (thr);
  TEST_VERIFY (xpthread_join (thr) == PTHREAD_CANCELED);

  restore_nofile ();
}

/* A failure to load the unwinder does not make pthread_create fail,
   and the unwinder is still loaded on first use.  */
static void
test_preload_failure (void *closure)
{
  TEST_VERIFY (!unwinder_loaded ());

  restrict_nofile ();
  pthread_t thr;
  TEST_COMPARE (pthread_create (&thr, NULL, thread_return, NULL), 0);
  xpthread_join (thr);
  restore_nofile ();

  TEST_VERIFY (!unwinder_loaded ());

  void *r = xpthread_join (xpthread_create (NULL, thread_exit,
					    (void *) (uintptr_t) 42));
  TEST_VERIFY (r == (void *) (uintptr_t) 42);
  TEST_VERIFY (unwinder_loaded ());

  thr = xpthread_create (NULL, thread_block, NULL);
  xpthread_cancel (thr);
  TEST_VERIFY (xpthread_join (thr) == PTHREAD_CANCELED);
}

static int
do_test (void)
{
  /* The tests below require a single-threaded process that has not
     loaded the unwinder yet.  */
  if (unwinder_loaded ())
    FAIL_UNSUPPORTED (UNWIND_SONAME " is already loaded");

  pid_t pid = xfork ();
  if (pid == 0)
    _exit (dlopen (UNWIND_SONAME, RTLD_LAZY) == NULL);
  int status;
  xwaitpid (pid, &status, 0);
  if (status != 0)
    FAIL_UNSUPPORTED (UNWIND_SONAME " cannot be loaded");

  support_isolate_in_subprocess (test_preload, NULL);
  support_isolate_in_subprocess (test_preload_failure, NULL);

  return 0;
}

#include <support/test-driver.c>
