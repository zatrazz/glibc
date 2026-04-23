/* Test program for argp argument parser
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
#include <time.h>
#include <string.h>
#include <argp.h>
#include <libintl.h>
#include <locale.h>
#include <unistd.h>
#include <support/support.h>
#include <support/check.h>

/* Note that the final invocation of argp --help will terminate the
   program with exit code 0.  */

#define PN_(ctxt, str) (str)
#define N_(str) (str)

const char *argp_program_version = "argphelp-test 1.0";

const struct argp_option options[] =
{
  {PN_ ("command-line option", "color"), 'c', N_ ("HUE"), 0, "Rainbow!"},
  {PN_ ("command-line option", "flavor"), 'f',
   N_ ("COOKIE"), OPTION_ARG_OPTIONAL, "Sweet!"},
  {PN_ ("command-line option", "texture"), 't', 0, 0, "Smooth!"},
  {0}
};

static bool color_set = false;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  (void) state;
  if (key == 'c' && color_set)
    FAIL ("color already set.\n");
  else if (key == 'c')
    color_set = true;
  return 0;
}

static const struct argp argp = { options, parse_opt };

static int
do_test (void)
{
  char *test1_argv[3] =
    { (char *) "/bin/tst-argphelp-localized", (char *) "--colour=yellow", NULL };
  char *test2_argv[3] =
    { (char *) "/bin/tst-argphelp-localized", (char *) "--help", NULL };

  unsetenv ("LANGUAGE");
  xsetlocale (LC_ALL, "en_GB.UTF-8");
  TEST_VERIFY_EXIT (bindtextdomain ("tst-argphelp-localized",
				    OBJPFX "domaindir") != NULL);
  TEST_VERIFY_EXIT (textdomain ("tst-argphelp-localized") != NULL);
  /* Check that the catalog is OK: */
  TEST_COMPARE_STRING (gettext ("command-line option\004color"), "colour");
  TEST_COMPARE_STRING (gettext ("COOKIE"), "BISCUIT");
  argp_parse (&argp, 2, test1_argv, 0, 0, NULL);
  TEST_VERIFY (color_set);
  color_set = false;

  /* This is the last chance to fail.  */
  if (support_record_failure_is_failed ())
    FAIL_EXIT1 ("There were test failures before the final invocation of --help");
  /* This last test will exit the program with code 0 and ignore
     previous failures.  */
  argp_parse (&argp, 2, test2_argv, 0, 0, NULL);
  FAIL_EXIT1 ("--help did not exit the program");
  return 0;
}

#define TEST_FUNCTION do_test
#include <support/test-driver.c>
