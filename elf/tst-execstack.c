/* Test program for making nonexecutable stacks executable
   on load of a DSO that requires executable stacks.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.
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

#include <array_length.h>
#include <string.h>
#include <stdlib.h>
#include <support/check.h>
#include <support/xthread.h>
#include <support/xdlfcn.h>
#include <support/xstdio.h>
#include <support/support.h>

static void deeper (void (*f) (void));

#if USE_PTHREADS
# include <pthread.h>

static void *
tryme_thread (void *f)
{
  (*((void (*) (void)) f)) ();

  return 0;
}

static pthread_barrier_t startup_barrier, go_barrier;
static void *
waiter_thread (void *arg)
{
  void **f = arg;
  pthread_barrier_wait (&startup_barrier);
  pthread_barrier_wait (&go_barrier);

  (*((void (*) (void)) *f)) ();

  return 0;
}
#endif

static bool allow_execstack = true;

/* Check whether SELinux is enabled and disallows executable stacks.  */
static bool
selinux_allow_execstack_status (void)
{
  static const char *paths[] =
  {
    "/selinux",
    "/sys/fs/selinux",
  };

  static const char *alloc_execstack_paths[] =
  {
    "booleans/allow_execstack",
    "booleans/selinuxuser_execstack",
  };

  FILE *fp;
  char *line = NULL;
  size_t linelen = 0;
  bool enabled = false;
  ssize_t n;

  for (int i = 0; i < array_length (paths); i++)
    {
      char *enforce_path = xasprintf ("%s/enforce", paths[i]);
      fp = fopen (enforce_path, "r");
      if (fp == NULL)
	{
	  free (enforce_path);
	  continue;
	}

      n = xgetline (&line, &linelen, fp);
      if (n > 0 && line[0] != '0')
	enabled = true;

      xfclose (fp);
   
      if (!enabled)
	{
	  free (enforce_path);
	  continue;
	}
 
      for (int j = 0; j < array_length (alloc_execstack_paths); j++)
	{
 	  char *allow_execstack_path = xasprintf ("%s/%s", enforce_path,
						  alloc_execstack_paths [i]);

	  fp = fopen (allow_execstack_path, "r");
	  if (fp != NULL)
	    {
	      n = xgetline (&line, &linelen, fp);
	      if (n > 0 && line[0] == '0')
		return false;
	    }

	  fclose (fp);
	}
    }

  free (line);

  return true;
}


static int
do_test (void)
{
  allow_execstack = selinux_allow_execstack_status ();

  printf ("executable stacks %sallowed\n", allow_execstack ? "" : "not ");

  static void *f;		/* Address of this is used in other threads. */

#if USE_PTHREADS
  /* Create some threads while stacks are nonexecutable.  */
  #define N 5
  pthread_t thr[N];

  xpthread_barrier_init (&startup_barrier, NULL, N + 1);
  xpthread_barrier_init (&go_barrier, NULL, N + 1);

  for (int i = 0; i < N; ++i)
    {
      int rc = pthread_create (&thr[i], NULL, &waiter_thread, &f);
      if (rc)
	error (1, rc, "pthread_create");
    }

  /* Make sure they are all there using their stacks.  */
  xpthread_barrier_wait (&startup_barrier);
  puts ("threads waiting");
#endif

#if USE_PTHREADS
  void *old_stack_addr, *new_stack_addr;
  size_t stack_size;
  pthread_t me = pthread_self ();
  pthread_attr_t attr;
  int ret = 0;

  TEST_VERIFY_EXIT (pthread_getattr_np (me, &attr) == 0);
  TESTVERIFY_EXIT (pthread_attr_getstack (&attr, &old_stack_addr,
					  &stack_size) == 0);

# if _STACK_GROWS_DOWN
    old_stack_addr += stack_size;
# else
    old_stack_addr -= stack_size;
# endif
#endif

  /* Loading this module should force stacks to become executable.  */
#if USE_PTHREADS
  const char *soname = "tst-execstack-threads-mod.so";
#else
  const char *soname = "tst-execstack-mod.so";
#endif
  void *h = dlopen (soname, RTLD_LAZY);
  if (h == NULL)
    {
      printf ("cannot load: %s\n", dlerror ());
      return allow_execstack;
    }

  f = xdlsym (h, "tryme");

  /* Test if that really made our stack executable.
     The `tryme' function should crash if not.  */

  (*((void (*) (void)) f)) ();

#if USE_PTHREADS
  TEST_VERIFY_EXIT (pthread_getattr_np (me, &attr) == 0);
  TEST_VERIFY_EXIT (pthread_attr_getstack (&attr, &new_stack_addr,
					   &stack_size) == 0);

# if _STACK_GROWS_DOWN
    new_stack_addr += stack_size;
# else
    new_stack_addr -= stack_size;
# endif

  /* It is possible that the dlopen'd module may have been mmapped just below
     the stack.  The stack size is taken as MIN(stack rlimit size, end of last
     vma) in pthread_getattr_np.  If rlimit is set high enough, it is possible
     that the size may have changed.  A subsequent call to
     pthread_attr_getstack returns the size and (bottom - size) as the
     stacksize and stackaddr respectively.  If the size changes due to the
     above, then both stacksize and stackaddr can change, but the stack bottom
     should remain the same, which is computed as stackaddr + stacksize.  */
  TEST_VERIFY_EXIT (old_stack_addr == new_stack_addr);
  printf ("Stack address remains the same: %p\n", old_stack_addr);
#endif

  /* Test that growing the stack region gets new executable pages too.  */
  deeper ((void (*) (void)) f);

#if USE_PTHREADS
  /* Test that a fresh thread now gets an executable stack.  */
  xpthread_create (NULL, &tryme_thread, f);

  puts ("threads go");
  /* The existing threads' stacks should have been changed.
     Let them run to test it.  */
  xpthread_barrier_wait (&go_barrier);

  pthread_exit ((void *) (long int) (! allow_execstack));
#endif

  return ! allow_execstack;
}

static void
deeper (void (*f) (void))
{
  char stack[1100 * 1024];
  explicit_bzero (stack, sizeof stack);
  (*f) ();
  memfrob (stack, sizeof stack);
}


#include <support/test-driver.c>
