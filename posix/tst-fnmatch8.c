/* Test fnmatch with invalid and mixed valid/invalid multibyte sequences.
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

/* In a multibyte locale fnmatch decodes the pattern and the string one
   logical character at a time, mapping each byte that does not start a
   valid sequence to a private marker (FNM_BAD_BYTE_BASE + byte) that only
   matches an identical byte.  These tests stress the behavior, in particular
   for inputs that interleave valid multibyte characters with stray or
   incomplete bytes, so the marker pass alone keeps producing the expected
   results.  */

#include <fnmatch.h>
#include <locale.h>
#include <support/check.h>

/* U+00E9 (e with acute), valid two-byte UTF-8 sequence.  */
#define EACUTE "\xc3\xa9"
/* 0xFF never appears in a valid UTF-8 sequence.  */
#define BAD "\xff"
/* A different stray byte.  */
#define BAD2 "\xfe"
/* Lone lead byte of a two-byte sequence: a valid prefix with no
   continuation, i.e. an incomplete sequence at end of string.  */
#define INCOMPLETE "\xc3"

static int
do_test (void)
{
  TEST_VERIFY_EXIT (setlocale (LC_ALL, "en_US.UTF-8") != NULL);

  /* '?' matches exactly one logical character, valid or not.  */
  TEST_VERIFY (fnmatch ("?", EACUTE, 0) == 0);
  TEST_VERIFY (fnmatch ("?", BAD, 0) == 0);
  TEST_VERIFY (fnmatch ("?", INCOMPLETE, 0) == 0);
  TEST_VERIFY (fnmatch ("?", "ab", 0) == FNM_NOMATCH);

  /* A stray byte only matches the identical stray byte.  */
  TEST_VERIFY (fnmatch (BAD, BAD, 0) == 0);
  TEST_VERIFY (fnmatch (BAD, BAD2, 0) == FNM_NOMATCH);
  TEST_VERIFY (fnmatch (INCOMPLETE, INCOMPLETE, 0) == 0);

  /* A valid multibyte character never matches a stray byte, and vice
     versa.  */
  TEST_VERIFY (fnmatch (EACUTE, BAD, 0) == FNM_NOMATCH);
  TEST_VERIFY (fnmatch (BAD, EACUTE, 0) == FNM_NOMATCH);

  /* Mixed valid + invalid input: a literal valid character followed by a
     '?' that lands on a stray byte.  */
  TEST_VERIFY (fnmatch (EACUTE "?", EACUTE BAD, 0) == 0);

  /* '*' may span stray bytes while still anchoring on a later literal.  */
  TEST_VERIFY (fnmatch ("a*b", "a" BAD "b", 0) == 0);
  TEST_VERIFY (fnmatch ("a*", "a" BAD BAD2, 0) == 0);
  TEST_VERIFY (fnmatch ("*" EACUTE, BAD EACUTE, 0) == 0);

  /* A stray byte is not a member of any character class.  */
  TEST_VERIFY (fnmatch ("[[:alpha:]]", BAD, 0) == FNM_NOMATCH);
  TEST_VERIFY (fnmatch ("[[:digit:]]", BAD, 0) == FNM_NOMATCH);

  /* Bracket membership for stray bytes: a stray byte matches neither a
     listed ASCII character nor a range, but a negated bracket accepts it.  */
  TEST_VERIFY (fnmatch ("[a]", BAD, 0) == FNM_NOMATCH);
  TEST_VERIFY (fnmatch ("[a-z]", BAD, 0) == FNM_NOMATCH);
  TEST_VERIFY (fnmatch ("[!a]", BAD, 0) == 0);

  return 0;
}

#include <support/test-driver.c>
