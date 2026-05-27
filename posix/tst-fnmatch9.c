/* Test for fnmatch '?' matching only one multibyte character (bug 31075).
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
#include <support/check.h>
#include <support/support.h>

static int
do_test (void)
{
  xsetlocale (LC_ALL, "en_US.UTF-8");

  /* '?' must match exactly one character (not one byte): a 2-byte UTF-8
     sequence ('é') is one character, and "??" therefore must not match
     a single 2-byte char.  */
  TEST_VERIFY (fnmatch ("??", "\xc3\xa9", 0) == FNM_NOMATCH);
  TEST_VERIFY (fnmatch ("??", "\xc3\xa9\xc3\xa9", 0) == 0);
  /* Two 4-byte UTF-8 sequences should still match "??".  */
  TEST_VERIFY (fnmatch ("??", "\xf4\x8f\xbf\xbf\xf4\x8f\xbf\xbf", 0) == 0);
  TEST_VERIFY (fnmatch ("?", "\xc3\xa9", 0) == 0);

  /* A range outside what '\xc3\xa9' (U+00E9) covers must not match it.  */
  TEST_VERIFY (fnmatch ("*[\xc3\xa1-\xc3\xa4]*", "\xc3\xa9", 0)
               == FNM_NOMATCH);
  TEST_VERIFY (fnmatch ("*[\xc3\xa1-\xc3\xa4]*", "\xc3\xa9\xc3\xa9", 0)
               == FNM_NOMATCH);

  /* Sanity checks: range that does cover U+00E9 should match.  */
  TEST_VERIFY (fnmatch ("[\xc3\xa1-\xc3\xaa]", "\xc3\xa9", 0) == 0);

  return 0;
}

#include <support/test-driver.c>
