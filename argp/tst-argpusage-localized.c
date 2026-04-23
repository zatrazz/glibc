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

/* Note that the final invoking argp --usage will terminate the
   program with exit code 0.  */

#define PN_(ctxt, str) (str)
#define N_(str) (str)

const char *argp_program_version = "argpusage-test 1.0";

const struct argp_option options[] =
{
  {PN_ ("command-line option", "color"), 'c', 0, 0, "Rainbow!"},
  {PN_ ("command-line option", "flavor"), 'f',
   N_ ("COOKIE"), OPTION_ARG_OPTIONAL, "Sweet!"},
  {0}
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  return 0;
}

static const struct argp argp = { options, parse_opt };

static int
do_test (void)
{
  char *test_argv[3] =
    { (char *) "/bin/tst-argpusage-localized", (char *) "--usage", NULL };

  unsetenv ("LANGUAGE");
  xsetlocale (LC_ALL, "en_GB.UTF-8");
  /* We reuse the tst-argphelp-localized domain to avoid making a new
     PO file.  */
  TEST_VERIFY_EXIT (bindtextdomain ("tst-argphelp-localized",
				    OBJPFX "domaindir") != NULL);
  TEST_VERIFY_EXIT (textdomain ("tst-argphelp-localized") != NULL);
  /* Check that the catalog is OK: */
  TEST_COMPARE_STRING (gettext ("command-line option\004color"), "colour");
  TEST_COMPARE_STRING (gettext ("COOKIE"), "BISCUIT");
  /* This is the last chance to fail.  */
  if (support_record_failure_is_failed ())
    FAIL_EXIT1 (
	"There were test failures before the final invocation of --usage");
  /* This last test will exit the program with code 0 and ignore
     previous failures.  */
  argp_parse (&argp, 2, test_argv, 0, 0, NULL);
  FAIL_EXIT1 ("--usage did not exit the program");
  return 0;
}

#define TEST_FUNCTION do_test
#include <support/test-driver.c>
