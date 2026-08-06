/* Test posix_close.
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
#include <fcntl.h>
#include <support/check.h>
#include <support/temp_file.h>
#include <support/xunistd.h>
#include <unistd.h>

static int
do_test (void)
{
  int fd = create_temp_file ("tst-posix_close.", NULL);
  TEST_VERIFY_EXIT (fd >= 0);

  /* A successful close: the descriptor is released.  */
  TEST_COMPARE (posix_close (fd, 0), 0);
  errno = 0;
  TEST_COMPARE (fcntl (fd, F_GETFD), -1);
  TEST_COMPARE (errno, EBADF);

  /* Closing an already closed descriptor fails with EBADF.  */
  errno = 0;
  TEST_COMPARE (posix_close (fd, 0), -1);
  TEST_COMPARE (errno, EBADF);

  errno = 0;
  TEST_COMPARE (posix_close (-1, 0), -1);
  TEST_COMPARE (errno, EBADF);

  /* POSIX_CLOSE_RESTART is 0 and is accepted.  */
  fd = xopen ("/dev/null", O_RDONLY, 0);
  TEST_COMPARE (posix_close (fd, POSIX_CLOSE_RESTART), 0);

  return 0;
}

#include <support/test-driver.c>
