/* Tests for posix_spawn resource limit set.
   Copyright (C) 2025 Free Software Foundation, Inc.
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
   <http://www.gnu.org/licenses/>.  */

#include <array_length.h>
#include <errno.h>
#include <getopt.h>
#include <intprops.h>
#include <inttypes.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <support/check.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <tst-spawn.h>
#include <unistd.h>

#define CMDLINE_OPTIONS \
  { "restart", no_argument, &restart, 1 },
static int restart;

static char *spargs[4                /* The four initial arguments.  */
		    + 1              /* The extra --direct.  */
		    + 1              /* The extra --restart.  */
		    + RLIM_NLIMITS   /* The resources to check.  */
		    + RLIM_NLIMITS   /* The soft limit.  */
		    + RLIM_NLIMITS   /* The maximum limit.  */
		    + 1              /* The final NULL.  */];
static int start_args;

_Noreturn static void
handle_restart (int argc, char *argv[])
{
  TEST_VERIFY_EXIT (argc % 3 == 0);

  for (int i = 0; i < argc; i += 3)
    {
      int resource;
      {
	char *endp;
	resource = strtol (argv[0], &endp, 10);
	TEST_VERIFY (endp != argv[0]);
      }
      TEST_VERIFY_EXIT (resource < RLIM_NLIMITS);
      rlim64_t cur;
      {
	char *endp;
	cur = strtoumax (argv[1], &endp, 10);
	TEST_VERIFY (endp != argv[1]);
      }
      rlim64_t max;
      {
	char *endp;
	max = strtoumax (argv[2], &endp, 10);
	TEST_VERIFY (endp != argv[2]);
      }

      struct rlimit rlimit;
      TEST_VERIFY_EXIT (getrlimit (resource, &rlimit) == 0);
      TEST_COMPARE (rlimit.rlim_cur, cur);
      TEST_COMPARE (rlimit.rlim_max, max);
    }

  exit (EXIT_SUCCESS);
}

static int
do_basic_test (void)
{
  for (int i = 0; i < RLIM_NLIMITS; i++)
    {
      posix_spawnattr_t attr;

      TEST_COMPARE (posix_spawnattr_init (&attr), 0);

      {
	/* Query a non initialized resource return EINVAL.  */
	struct rlimit rlim;
	TEST_COMPARE (posix_spawnattr_getrlimit_np (&attr, i, &rlim),
		      EINVAL);
      }

      TEST_COMPARE (posix_spawnattr_setrlimit_np (&attr, i,
						  &(const struct rlimit)
						   { 0, RLIM_INFINITY }),
		    0);
      {
	struct rlimit rlim;
	TEST_COMPARE (posix_spawnattr_getrlimit_np (&attr, i, &rlim),
		      0);
	TEST_COMPARE (rlim.rlim_cur, 0);
	TEST_COMPARE (rlim.rlim_max, RLIM_INFINITY);
      }

      /* Overwrite the resource value.  */
      TEST_COMPARE (posix_spawnattr_setrlimit_np (&attr, i,
						  &(const struct rlimit)
						   { RLIM_INFINITY, 0 }),
		    0);
      {
	struct rlimit rlim;
	TEST_COMPARE (posix_spawnattr_getrlimit_np (&attr, i, &rlim),
		      0);
	TEST_COMPARE (rlim.rlim_cur, RLIM_INFINITY);
	TEST_COMPARE (rlim.rlim_max, 0);
      }

      TEST_COMPARE (posix_spawnattr_destroy (&attr), 0);
    }

  return 0;
}

static int
do_test_spawn (void)
{
  struct rlimit rlimits[RLIM_NLIMITS];
  for (int i = 0; i < RLIM_NLIMITS; i++)
    TEST_VERIFY_EXIT (getrlimit (i, &rlimits[i]) == 0);

  char resource_str[RLIM_NLIMITS][INT_STRLEN_BOUND (int)];
  char cur_limit_str[RLIM_NLIMITS][INT_STRLEN_BOUND (uintmax_t)];
  char max_limit_str[RLIM_NLIMITS][INT_STRLEN_BOUND (uintmax_t)];

  posix_spawnattr_t attr;
  TEST_COMPARE (posix_spawnattr_init (&attr), 0);

  int argc = start_args;
  for (int i = 0; i < RLIM_NLIMITS; i++)
    {
      if (rlimits[i].rlim_max == 0)
	continue;

      rlimits[i].rlim_cur /= 2;
      rlimits[i].rlim_max /= 2;

      TEST_COMPARE (posix_spawnattr_setrlimit_np (&attr, i, &rlimits[i]), 0);

      snprintf (resource_str[i], sizeof resource_str[i], "%d", i);
      snprintf (cur_limit_str[i], sizeof cur_limit_str[i], "%ju",
		(uintmax_t) rlimits[i].rlim_cur);
      snprintf (max_limit_str[i], sizeof max_limit_str[i], "%ju",
		(uintmax_t) rlimits[i].rlim_max);

      spargs[argc++] = resource_str[i];
      spargs[argc++] = cur_limit_str[i];
      spargs[argc++] = max_limit_str[i];
    }
  spargs[argc] = NULL;

  TEST_COMPARE (posix_spawnattr_setflags (&attr, POSIX_SPAWN_SETRLIMIT), 0);

  PID_T_TYPE pid;
  TEST_COMPARE (POSIX_SPAWN (&pid, spargs[0], NULL, &attr, spargs, environ),
		0);
  TEST_COMPARE (posix_spawnattr_destroy (&attr), 0);

  siginfo_t sinfo;
  TEST_COMPARE (WAITID (P_ALL, 0, &sinfo, WEXITED), 0);
  TEST_COMPARE (sinfo.si_code, CLD_EXITED);
  TEST_COMPARE (sinfo.si_status, 0);

  return 0;
}

static int
do_test (int argc, char *argv[])
{
  if (restart)
    handle_restart (argc - 1, &argv[1]);
  TEST_VERIFY_EXIT (argc == 2 || argc == 5);

  do_basic_test ();

  {
    int i;
    for (i = 0; i < argc - 1; i++)
      spargs[i] = argv[i + 1];
    spargs[i++] = (char *) "--direct";
    spargs[i++] = (char *) "--restart";
    start_args = i;
  }

  do_test_spawn ();

  return 0;
}


#define TEST_FUNCTION_ARGV do_test
#include <support/test-driver.c>
