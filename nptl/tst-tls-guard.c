/* Tests for the TLS guard page between stack and static TLS.
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

/* For architectures that set ARCH_HAS_TLS_GUARD, glibc inserts a guard page
   between the thread stack and the static TLS / struct pthread area.
   This test verifies:
     1. The TLS guard is inaccessible (traps on read/write).
     2. The stack area below the TLS guard is accessible.
     3. When guardsize == 0 no TLS guard is added.
     4. The behaviour is consistent across kernel support for
        MADV_GUARD_INSTALL and the PROT_NONE fallback.  */

#include <array_length.h>
#include <pthread.h>
#include <pthreaddef.h>
#include <setjmp.h>
#include <stackinfo.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <support/capture_subprocess.h>
#include <support/check.h>
#include <support/check_mem_access.h>
#include <support/test-driver.h>
#include <support/xsignal.h>
#include <support/xthread.h>
#include <support/xunistd.h>
#include <tls.h>

#if ARCH_HAS_TLS_GUARD

static long int pagesz;

/* Locate the TLS guard region by probing pages backward from STACK_TOP
   (which equals (char *)stack + stacksize from pthread_attr_getstack, i.e.
   approximately the first byte above the TLS guard) down to SCAN_LIMIT.
   Works for both MADV_GUARD_INSTALL (guard stays inside one RW VMA but
   pages are inaccessible) and the PROT_NONE fallback (separate ---p VMA).
   Returns 1 and fills the output pointers on success, 0 if no inaccessible
   region is found within the scan range.  */
static int
find_tls_guard (char *stack_top, char *scan_limit, long pagesize,
                char **guard_start_out, size_t *guard_size_out)
{
  char *guard_end = NULL;
  char *guard_start = NULL;
  char *p = stack_top;
  /* Probe at most 128 pages.  The TLS guard sits within a few pages of
     stack_top (static-TLS alignment overhead is typically 1–2 pages);
     128 pages gives ample headroom for large-TLS workloads.  */
  char *stop = stack_top - 128 * pagesize;
  if (stop < scan_limit)
    stop = scan_limit;

  while (p > stop)
    {
      p -= pagesize;
      if (!check_mem_access (p, false))
        {
          if (guard_end == NULL)
            guard_end = p + pagesize;
          guard_start = p;
        }
      else if (guard_end != NULL)
        break;
    }

  if (guard_end == NULL)
    return 0;

  *guard_start_out = guard_start;
  *guard_size_out  = (size_t) (guard_end - guard_start);
  return 1;
}

struct thread_args
{
  int expect_tls_guard;  /* 1: TLS guard expected, 0: not expected */
};

static void *
tf (void *closure)
{
  struct thread_args *args = closure;

  pthread_attr_t attr;
  TEST_VERIFY_EXIT (pthread_getattr_np (pthread_self (), &attr) == 0);

  void *stack;
  size_t stacksize;
  TEST_VERIFY_EXIT (pthread_attr_getstack (&attr, &stack, &stacksize) == 0);

  size_t guardsize;
  TEST_VERIFY_EXIT (pthread_attr_getguardsize (&attr, &guardsize) == 0);
  if (guardsize != 0 && guardsize < ARCH_MIN_GUARD_SIZE)
    guardsize = ARCH_MIN_GUARD_SIZE;
  pthread_attr_destroy (&attr);

  /* Verify the stack area is accessible (start and midpoint are always in
     the usable stack region below the TLS guard).  */
  TEST_VERIFY (check_mem_access (stack, false));
  TEST_VERIFY (check_mem_access ((char *) stack + stacksize / 4, false));

  /* pthread_getattr_np reports stacksize as the user-requested size (both
     the stack guard and the TLS guard are excluded) and stackaddr as the
     top of that region.  For _STACK_GROWS_DOWN:
       stack              == stackblock + guardsize  (low end of usable stack)
       stack + stacksize + guardsize == end of the mmap'd allocation
     Scan from the end of the allocation downward; the TLS guard is the
     first inaccessible region encountered.  */
  char *scan_top = (char *) stack + stacksize + guardsize;
  char  *guard_start = NULL;
  size_t guard_size  = 0;
  int    found = find_tls_guard (scan_top, (char *) stack,
                                 pagesz, &guard_start, &guard_size);

  if (args->expect_tls_guard)
    {
      TEST_VERIFY (found != 0);
      if (!found)
        return NULL;

      if (test_verbose)
        printf ("debug: [tid=%jd] TLS guard at %p+%#zx, guardsize=%#zx\n",
                (intmax_t) gettid (), guard_start, guard_size, guardsize);

      /* The TLS guard should be the same size as the stack guard.  */
      TEST_COMPARE (guard_size, guardsize);

      /* Every byte of the TLS guard must be inaccessible.  */
      TEST_VERIFY (!check_mem_access (guard_start, false));
      TEST_VERIFY (!check_mem_access (guard_start, true));
      TEST_VERIFY (!check_mem_access (guard_start + guard_size / 2, false));
      TEST_VERIFY (!check_mem_access (guard_start + guard_size - 1, false));

      /* The byte just below the TLS guard must be accessible (stack area).  */
      TEST_VERIFY (check_mem_access (guard_start - 1, false));
    }
  else
    {
      /* With guardsize == 0 there must be no TLS guard.  */
      TEST_VERIFY (found == 0);
    }

  return NULL;
}

/* Test with default pthread attributes (guard present).  */
static void
do_test_default (void *closure)
{
  struct thread_args args = {
    .expect_tls_guard = 1
  };
  pthread_t t = xpthread_create (NULL, tf, &args);
  void *status = xpthread_join (t);
  TEST_VERIFY (status == 0);
}

/* Test with explicit guard size > 0 (guard present).  */
static void
do_test_with_guard (void *closure)
{
  pthread_attr_t attr;
  xpthread_attr_init (&attr);
  xpthread_attr_setstacksize (&attr, 2 * 1024 * 1024);
  xpthread_attr_setguardsize (&attr, pagesz);

  struct thread_args args = {
    .expect_tls_guard = 1
  };
  pthread_t t = xpthread_create (&attr, tf, &args);
  void *status = xpthread_join (t);
  TEST_VERIFY (status == 0);

  xpthread_attr_destroy (&attr);
}

/* Test with guardsize == 0 (no TLS guard expected).  */
static void
do_test_no_guard (void *closure)
{
  pthread_attr_t attr;
  xpthread_attr_init (&attr);
  /* Use a larger stack so the thread still fits after removing the guard.  */
  xpthread_attr_setstacksize (&attr, 2 * 1024 * 1024);
  xpthread_attr_setguardsize (&attr, 0);

  struct thread_args args = {
    .expect_tls_guard = 0
  };
  pthread_t t = xpthread_create (&attr, tf, &args);
  void *status = xpthread_join (t);
  TEST_VERIFY (status == 0);

  xpthread_attr_destroy (&attr);
}

/* Test guard after cache reuse: create without guard, then with guard.  */
static void
do_test_cache_reuse (void *closure)
{
  /* First thread: no guard, puts stack in cache.  */
  do_test_no_guard (NULL);

  /* Second thread: default guard, retrieves cached stack and installs guard.  */
  do_test_default (NULL);

  /* Third thread: larger guard, exercises guard expansion.  */
  pthread_attr_t attr;
  xpthread_attr_init (&attr);
  xpthread_attr_setstacksize (&attr, 2 * 1024 * 1024);
  xpthread_attr_setguardsize (&attr, 2 * pagesz);

  struct thread_args args = {
    .expect_tls_guard = 1
  };
  pthread_t t = xpthread_create (&attr, tf, &args);
  xpthread_join (t);
  xpthread_attr_destroy (&attr);
}
#endif /* ARCH_HAS_TLS_GUARD */

static int
do_test (void)
{
#if ARCH_HAS_TLS_GUARD
  pagesz = sysconf (_SC_PAGESIZE);

  static const struct
  {
    const char *descr;
    void (*fn) (void *);
  } tests[] = {
    { "default attributes (TLS guard expected)",       do_test_default     },
    { "explicit guard size > 0 (TLS guard expected)",  do_test_with_guard  },
    { "guardsize == 0 (no TLS guard expected)",        do_test_no_guard    },
    { "cache reuse: no-guard then with-guard",         do_test_cache_reuse },
  };

  /* Run each sub-test in a forked subprocess for isolation.  */
  for (size_t i = 0; i < array_length (tests); i++)
    {
      printf ("info: fork: %s\n", tests[i].descr);
      struct support_capture_subprocess r =
        support_capture_subprocess (tests[i].fn, NULL);
      support_capture_subprocess_check (&r, tests[i].descr, 0,
                                        sc_allow_none);
      support_capture_subprocess_free (&r);
    }

  /* Then run them without fork so the stack cache is exercised.  */
  for (size_t i = 0; i < array_length (tests); i++)
    {
      printf ("info: %s\n", tests[i].descr);
      tests[i].fn (NULL);
    }

  return 0;
#else
  puts ("SKIP: TLS guard not enabled for this architecture");
  return EXIT_SUCCESS;
#endif
}

#include <support/test-driver.c>
