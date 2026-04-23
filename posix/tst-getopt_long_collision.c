/* Copyright (C) 2026 Free Software Foundation, Inc.
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#include <locale.h>
#include <support/support.h>
#include <support/check.h>

#define PN_(ctxt, str) (str)

/* There are 2 types of collision that can happen: a translation equal
   to an existing option, or two different options translating to the
   same thing.

   We test both kinds.  In the first test, we have this setup:
   foo -> bar
   bar -> baz

   In the second test, we have this setup:
   foo -> same
   bar -> same

   In the third, we don’t translate anything:
   foo -> foo
   bar -> bar
  */

static const struct option options[] =
  {
    {"foo", no_argument, NULL, 'f'},
    {"bar", no_argument, NULL, 'b'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

static void
setup_catalog (void)
{
  xsetlocale (LC_MESSAGES, "fr_FR.UTF-8");
  TEST_VERIFY_EXIT (
      bindtextdomain ("tst-getopt_long_collision", OBJPFX "domaindir")
      != NULL);
  TEST_VERIFY_EXIT (textdomain ("tst-getopt_long_collision") != NULL);
  /* Check that the catalog is OK: */
  TEST_COMPARE_STRING (dgettext ("tst-getopt_long_collision", "kind 1\004foo"),
		       "bar");
  TEST_COMPARE_STRING (dgettext ("tst-getopt_long_collision", "kind 1\004bar"),
		       "baz");
  TEST_COMPARE_STRING (dgettext ("tst-getopt_long_collision", "kind 2\004foo"),
		       "same");
  TEST_COMPARE_STRING (dgettext ("tst-getopt_long_collision", "kind 2\004bar"),
		       "same");
  TEST_COMPARE_STRING (dgettext ("tst-getopt_long_collision", "kind 3\004foo"),
		       "kind 3\004foo");
  TEST_COMPARE_STRING (dgettext ("tst-getopt_long_collision", "kind 3\004bar"),
		       "kind 3\004bar");
}

static void
do_test (int kind, int expected, int optind_after_first_run,
	 int expected_second_run)
{
  /* Check test --help with one of the 3 tests.  We expect the first
     call to getopt_long to return expected, while setting optind to
     optind_after_first_run.  We call getopt_long a second time, and
     we expect it to return expected_second_run, while setting optind
     to 2.  */
  static const char *contexts[] = { "kind 1", "kind 2", "kind 3" };
  const char *context = contexts[kind];
  int c;
  int option_index = 0;
  const static char *argv[] =
    { (char *) "tst-getopt_long_collision", "--help", NULL };
  const static int argc = 2;
  optind = 0;
  TEST_VERIFY_EXIT (getopt_long_enable_translations (context, NULL) == 0);
  fprintf (stderr, "Start test %d.\n", kind + 1);
  /* First pass should detect the problem immediately, even if we do
     not trigger the option.  */
  c = getopt_long (argc, (char **) argv, "fbh", options, &option_index);
  TEST_COMPARE (c, expected);
  TEST_COMPARE (optind, optind_after_first_run);
  /* The translations check is only run once. */
  fprintf (stderr, "Restart test %d, we expect no problems.\n", kind + 1);
  c = getopt_long (argc, (char **) argv, "fbh", options, &option_index);
  TEST_COMPARE (c, expected_second_run);
  TEST_COMPARE (optind, 2);
}

static int
do_all_tests (void)
{
  setup_catalog ();
  /* In failure cases, the first time we parse, we should get '?', and
     optind stays at 1.  The second time, we parse the first option.

     In the normal case, the first time we parse, we should get the
     first option and optind jumps directly to 2.  The second time, we
     parsed everything.
  */
  do_test (0, '?', 1, 'h');
  do_test (1, '?', 1, 'h');
  do_test (2, 'h', 2, -1);
  getopt_long_disable_translations ();
  return 0;
}

#define TEST_FUNCTION do_all_tests
#include <support/test-driver.c>
