/* Tests for str2sig and sig2str.
   Copyright (C) 2024 Free Software Foundation, Inc.
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
#include <signal.h>
#include <support/support.h>
#include <support/check.h>

static const struct test_t
{
  int sig;
  const char *abbrev;
} tests[] =
{
#define N_(name)                      name
#define init_sig(sig, abbrev, desc)   { sig, abbrev },
#include <siglist.h>
#undef init_sig
};

#include <stdio.h>

static void
expected_rtsig_str (int signum, char *str)
{
  int base;
  if (signum <= SIGRTMIN + (SIGRTMAX - SIGRTMIN) / 2)
    {
      str = stpcpy (str, "RTMIN");
      base = SIGRTMIN;
    }
  else
    {
      str = stpcpy (str, "RTMAX");
      base = SIGRTMAX;
    }
  int delta = signum - base;
  if (delta != 0)
    sprintf (str, "%+d", delta);
}

static void
test_sig2str (void)
{
  char str[SIG2STR_MAX];

  TEST_COMPARE (sig2str (-1, str), -1);
  TEST_COMPARE (sig2str (SIGRTMAX+1, str), -1);
  TEST_COMPARE (sig2str (SIGRTMIN-1, str), -1);

  for (size_t i = 0; i < array_length (tests); i++)
    {
      TEST_COMPARE (sig2str (tests[i].sig, str), 0);
      TEST_COMPARE_STRING (str, tests[i].abbrev);
    }

  for (int rtsig = SIGRTMIN; rtsig <= SIGRTMAX; rtsig++)
    {
      char expected[SIG2STR_MAX];
      expected_rtsig_str (rtsig, expected);

      TEST_COMPARE (sig2str (rtsig, str), 0);
      TEST_COMPARE_STRING (str, expected);
    }
}

static void
test_str2sig (void)
{
  {
    int signum;
    TEST_COMPARE (str2sig ("invalid", &signum), -1);
    TEST_COMPARE (str2sig ("4096", &signum), -1);
    TEST_COMPARE (str2sig ("RTMIN-4096", &signum), -1);
    TEST_COMPARE (str2sig ("RTMIN+4096", &signum), -1);
    TEST_COMPARE (str2sig ("RTMAX-4096", &signum), -1);
    TEST_COMPARE (str2sig ("RTMAX+4096", &signum), -1);
  }

  for (size_t i = 0; i < array_length (tests); i++)
    {
      int signum;
      TEST_COMPARE (str2sig (tests[i].abbrev, &signum), 0);
      TEST_COMPARE (signum, tests[i].sig);
    }

  for (int rtsig = SIGRTMIN; rtsig <= SIGRTMAX; rtsig++)
    {
      char sigstr[SIG2STR_MAX];
      expected_rtsig_str (rtsig, sigstr);

      int signum;
      TEST_COMPARE (str2sig (sigstr, &signum), 0);
      TEST_COMPARE (rtsig, signum);
    }
}

static int
do_test (void)
{
  test_sig2str ();
  test_str2sig ();

  return 0;
}

#include <support/test-driver.c>
