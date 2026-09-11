/* Test wordexp expansion of $* or $@ without positional parameters (BZ 34608).
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

/* The testsuite runs this without positional parameters, so there is
   nothing for $* and $@ below to expand to.  */

#include <stdlib.h>
#include <wordexp.h>

#include <support/check.h>

static void
check_no_field (const char *words)
{
  wordexp_t we = { 0 };

  TEST_COMPARE (wordexp (words, &we, 0), 0);
  TEST_COMPARE (we.we_wordc, 0);

  wordfree (&we);
}

static void
check_one_field (const char *words, const char *expected)
{
  wordexp_t we = { 0 };

  TEST_COMPARE (wordexp (words, &we, 0), 0);
  TEST_COMPARE (we.we_wordc, 1);
  TEST_COMPARE_STRING (we.we_wordv[0], expected);

  wordfree (&we);
}

static int
do_test (void)
{
  /* Both spellings take the same code path when unquoted, and field
     splitting leaves nothing behind.  */
  check_no_field ("$*");
  check_no_field ("$@");

  /* ${#*} and ${#@} report the number of positional parameters.  */
  check_one_field ("${#*}", "0");
  check_one_field ("${#@}", "0");

  /* "$*" is not subject to field splitting, so it expands to a single
     null string.  */
  check_one_field ("\"$*\"", "");

  /* POSIX requires "$@" to generate zero fields, even though it is
     double-quoted.  */
  check_no_field ("\"$@\"");

  setenv ("var", "", 1);

  check_no_field ("\"$@$@\"");
  check_no_field ("\"$@\"\"$@\"");
  check_one_field ("\"\"", "");
  check_one_field ("\"\"\"$@\"", "");
  check_one_field ("\"$@\"\"\"", "");
  check_one_field ("\"$var$@\"", "");
  check_one_field ("\"$@$var\"", "");

  unsetenv ("var");

  return 0;
}

#include <support/test-driver.c>
