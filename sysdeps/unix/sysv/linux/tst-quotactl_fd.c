/* Basic tests for Linux quotactl_fd.
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

#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/quota.h>
#include <support/check.h>
#include <support/temp_file.h>
#include <support/xunistd.h>

/* Enabling quotas requires privileges (the kernel does not support them for
   tmpfs mounts in a user namespace), so only use basic tests.  */

static int
do_test (void)
{
  uint32_t fmt;

  /* The file descriptor is checked first.  */
  TEST_COMPARE (quotactl_fd (-1, QCMD (Q_GETFMT, USRQUOTA), 0, &fmt), -1);
  if (errno == ENOSYS)
    FAIL_UNSUPPORTED ("quotactl_fd is not supported by the kernel");
  TEST_COMPARE (errno, EBADF);

  char *dir = support_create_temp_directory ("tst-quotactl_fd.");
  int fd = xopen (dir, O_RDONLY | O_DIRECTORY, 0);
  TEST_COMPARE (quotactl_fd (fd, QCMD (Q_GETFMT, MAXQUOTAS), 0, &fmt), -1);
  TEST_COMPARE (errno, EINVAL);
  xclose (fd);

  free (dir);
  return 0;
}

#include <support/test-driver.c>
