/* Basic tests for Linux memfd_secret.
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
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <support/check.h>
#include <support/xunistd.h>

static int
do_test (void)
{
  int fd = memfd_secret (0);
  if (fd == -1 && errno == ENOSYS)
    FAIL_UNSUPPORTED ("memfd_secret is not supported by the system");
  TEST_VERIFY_EXIT (fd != -1);

  long int pagesize = sysconf (_SC_PAGESIZE);
  xftruncate (fd, pagesize);
  char *p = xmmap (NULL, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd);
  static const char secret[] = "secret";
  memcpy (p, secret, sizeof (secret));
  TEST_COMPARE_STRING (p, secret);

  /* The memory can only be accessed through a shared mapping.  */
  {
    char buf[sizeof (secret)];
    TEST_COMPARE (read (fd, buf, sizeof (buf)), -1);
    TEST_COMPARE (errno, EINVAL);
  }
  TEST_VERIFY (mmap (NULL, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd,
		     0) == MAP_FAILED);
  TEST_COMPARE (errno, EINVAL);

  xmunmap (p, pagesize);
  xclose (fd);

  fd = memfd_secret (O_CLOEXEC);
  TEST_VERIFY_EXIT (fd != -1);
  TEST_COMPARE (fcntl (fd, F_GETFD), FD_CLOEXEC);
  xclose (fd);

  /* O_CLOEXEC is the only supported flag.  */
  TEST_COMPARE (memfd_secret (O_NONBLOCK), -1);
  TEST_COMPARE (errno, EINVAL);

  return 0;
}

#include <support/test-driver.c>
