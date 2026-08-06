/* Test sig2str and str2sig.
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

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <support/check.h>

static void
check_roundtrip (int sig)
{
  char buf[SIG2STR_MAX];
  int num;
  TEST_COMPARE (sig2str (sig, buf), 0);
  TEST_VERIFY (strlen (buf) < SIG2STR_MAX);
  TEST_COMPARE (str2sig (buf, &num), 0);
  TEST_COMPARE (num, sig);
}

static int
do_test (void)
{
  char buf[SIG2STR_MAX];
  int num;

  /* All valid signal numbers round-trip through sig2str and str2sig,
     and their decimal representations are accepted as well.  */
  for (int sig = 1; sig <= SIGRTMAX; sig++)
    {
      TEST_COMPARE (sig2str (sig, buf), 0);
      TEST_VERIFY (strlen (buf) < SIG2STR_MAX);
      TEST_COMPARE (str2sig (buf, &num), 0);
      TEST_COMPARE (num, sig);

      char dec[SIG2STR_MAX];
      snprintf (dec, sizeof (dec), "%d", sig);
      TEST_COMPARE (str2sig (dec, &num), 0);
      TEST_COMPARE (num, sig);
    }

  /* Names of standard signals do not include the SIG prefix.  */
  TEST_COMPARE (sig2str (SIGHUP, buf), 0);
  TEST_COMPARE_STRING (buf, "HUP");
  TEST_COMPARE (sig2str (SIGKILL, buf), 0);
  TEST_COMPARE_STRING (buf, "KILL");
  TEST_COMPARE (sig2str (SIGTERM, buf), 0);
  TEST_COMPARE_STRING (buf, "TERM");

  TEST_COMPARE (str2sig ("HUP", &num), 0);
  TEST_COMPARE (num, SIGHUP);
  TEST_COMPARE (str2sig ("KILL", &num), 0);
  TEST_COMPARE (num, SIGKILL);

  /* Realtime signals.  */
  check_roundtrip (SIGRTMIN);
  check_roundtrip (SIGRTMAX);
  TEST_COMPARE (sig2str (SIGRTMIN, buf), 0);
  TEST_COMPARE_STRING (buf, "RTMIN");
  TEST_COMPARE (sig2str (SIGRTMAX, buf), 0);
  TEST_COMPARE_STRING (buf, "RTMAX");
  if (SIGRTMAX - SIGRTMIN >= 2)
    {
      check_roundtrip (SIGRTMIN + 1);
      check_roundtrip (SIGRTMAX - 1);
      TEST_COMPARE (sig2str (SIGRTMIN + 1, buf), 0);
      TEST_COMPARE_STRING (buf, "RTMIN+1");
      TEST_COMPARE (sig2str (SIGRTMAX - 1, buf), 0);
      TEST_COMPARE_STRING (buf, "RTMAX-1");

      TEST_COMPARE (str2sig ("RTMIN+1", &num), 0);
      TEST_COMPARE (num, SIGRTMIN + 1);
      TEST_COMPARE (str2sig ("RTMAX-1", &num), 0);
      TEST_COMPARE (num, SIGRTMAX - 1);
    }

  /* Invalid inputs to sig2str.  */
  TEST_COMPARE (sig2str (-1, buf), -1);
  TEST_COMPARE (sig2str (0, buf), -1);
  TEST_COMPARE (sig2str (SIGRTMAX + 1, buf), -1);
  TEST_COMPARE (sig2str (NSIG + 100, buf), -1);

  /* Invalid inputs to str2sig.  */
  TEST_COMPARE (str2sig ("", &num), -1);
  TEST_COMPARE (str2sig ("SIGHUP", &num), -1);
  TEST_COMPARE (str2sig ("bogus", &num), -1);
  TEST_COMPARE (str2sig ("hup", &num), -1);
  TEST_COMPARE (str2sig ("0", &num), -1);
  TEST_COMPARE (str2sig ("-1", &num), -1);
  TEST_COMPARE (str2sig ("7x", &num), -1);
  TEST_COMPARE (str2sig ("12345678901234567890", &num), -1);
  TEST_COMPARE (str2sig ("RTMIN+0", &num), -1);
  TEST_COMPARE (str2sig ("RTMAX-0", &num), -1);
  TEST_COMPARE (str2sig ("RTMIN-1", &num), -1);
  TEST_COMPARE (str2sig ("RTMAX+1", &num), -1);
  TEST_COMPARE (str2sig ("RTMIN+999", &num), -1);
  TEST_COMPARE (str2sig ("RTMIN+1x", &num), -1);

  return 0;
}

#include <support/test-driver.c>
