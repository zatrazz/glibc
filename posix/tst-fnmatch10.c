/* Test for consistent fnmatch behavior with unknown character classes
   (bug 30483).
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

#include <fnmatch.h>
#include <locale.h>
#include <stdio.h>
#include <support/check.h>
#include <support/support.h>

/* When a bracket expression contains a character class that is not
   recognized in the current locale, the result of fnmatch must not
   depend on the position of that class within the bracket.  In the
   pattern "[x[:bogus:]]" the literal 'x' matches first; in
   "[[:bogus:]x]" the unknown class is scanned first.  Previously the
   second form returned FNM_NOMATCH because the implementation aborted
   the whole match on an unknown class name.  */

static void
check_pair (const char *string, const char *pattern_a, const char *pattern_b)
{
  int a = fnmatch (pattern_a, string, 0);
  int b = fnmatch (pattern_b, string, 0);
  TEST_VERIFY (a == b);
}

static void
do_test_locale (const char *locale)
{
  xsetlocale (LC_ALL, locale);

  /* Unknown class name "bogus" -- both forms must report a match.  */
  check_pair ("x", "[x[:bogus:]]", "[[:bogus:]x]");

  /* "hangul" is locale-specific; in non-Korean locales it is unknown
     while in ko_KR it is defined but does not match 'x'.  Either way
     the two patterns must agree.  */
  check_pair ("x", "[x[:hangul:]]", "[[:hangul:]x]");

  /* Three-entry bracket: the unknown class can appear at the start,
     middle, or end of the expression.  */
  check_pair ("x", "[x[:bogus:]y]", "[y[:bogus:]x]");
  check_pair ("x", "[[:bogus:]xy]", "[xy[:bogus:]]");
}

static int
do_test (void)
{
  do_test_locale ("C");
  do_test_locale ("en_US.UTF-8");
  do_test_locale ("ko_KR.UTF-8");
  return 0;
}

#include <support/test-driver.c>
