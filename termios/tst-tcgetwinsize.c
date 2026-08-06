/* Test tcgetwinsize and tcsetwinsize.
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

#include <errno.h>
#include <support/check.h>
#include <support/tty.h>
#include <support/xunistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static int
do_test (void)
{
  int outer, inner;
  struct winsize ws = { .ws_row = 24, .ws_col = 80 };
  support_openpty (&outer, &inner, NULL, NULL, &ws);

  /* The initial size is reported on both ends.  */
  struct winsize ws2 = { 0 };
  TEST_COMPARE (tcgetwinsize (inner, &ws2), 0);
  TEST_COMPARE (ws2.ws_row, 24);
  TEST_COMPARE (ws2.ws_col, 80);
  TEST_COMPARE (tcgetwinsize (outer, &ws2), 0);
  TEST_COMPARE (ws2.ws_row, 24);
  TEST_COMPARE (ws2.ws_col, 80);

  /* A changed size is visible on both ends and matches the TIOCGWINSZ
     ioctl.  */
  ws2.ws_row = 50;
  ws2.ws_col = 132;
  TEST_COMPARE (tcsetwinsize (inner, &ws2), 0);
  struct winsize ws3 = { 0 };
  TEST_COMPARE (tcgetwinsize (outer, &ws3), 0);
  TEST_COMPARE (ws3.ws_row, 50);
  TEST_COMPARE (ws3.ws_col, 132);
  struct winsize ws4 = { 0 };
  TEST_COMPARE (ioctl (inner, TIOCGWINSZ, &ws4), 0);
  TEST_COMPARE (ws4.ws_row, 50);
  TEST_COMPARE (ws4.ws_col, 132);

  /* Invalid file descriptor.  */
  errno = 0;
  TEST_COMPARE (tcgetwinsize (-1, &ws2), -1);
  TEST_COMPARE (errno, EBADF);
  errno = 0;
  TEST_COMPARE (tcsetwinsize (-1, &ws2), -1);
  TEST_COMPARE (errno, EBADF);

  /* Not a terminal.  */
  int fds[2];
  xpipe (fds);
  errno = 0;
  TEST_COMPARE (tcgetwinsize (fds[0], &ws2), -1);
  TEST_COMPARE (errno, ENOTTY);
  errno = 0;
  TEST_COMPARE (tcsetwinsize (fds[0], &ws2), -1);
  TEST_COMPARE (errno, ENOTTY);

  xclose (fds[0]);
  xclose (fds[1]);
  xclose (inner);
  xclose (outer);

  return 0;
}

#include <support/test-driver.c>
