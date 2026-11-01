/* Basic tests for Linux kcmp.
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
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/kcmp.h>
#include <sys/wait.h>
#include <unistd.h>
#include <support/check.h>
#include <support/xunistd.h>

static int
do_test (void)
{
  pid_t pid = getpid ();
  int fd1 = xopen ("/dev/null", O_RDONLY, 0);

  int r = kcmp (pid, pid, KCMP_FILE, fd1, fd1);
  if (r == -1 && errno == ENOSYS)
    FAIL_UNSUPPORTED ("kcmp is not supported by the kernel");
  TEST_COMPARE (r, 0);

  /* A duplicated descriptor refers to the same open file description.  */
  int fd2 = xdup (fd1);
  TEST_COMPARE (kcmp (pid, pid, KCMP_FILE, fd1, fd2), 0);

  /* A new open creates a new one, and the result orders both.  */
  int fd3 = xopen ("/dev/null", O_RDONLY, 0);
  r = kcmp (pid, pid, KCMP_FILE, fd1, fd3);
  TEST_VERIFY (r == 1 || r == 2);
  TEST_COMPARE (kcmp (pid, pid, KCMP_FILE, fd3, fd1), 3 - r);

  TEST_COMPARE (kcmp (pid, pid, KCMP_VM, 0, 0), 0);
  TEST_COMPARE (kcmp (pid, pid, KCMP_FILES, 0, 0), 0);

  /* Check if kcmp and file descriptor child sharing.  */
  int pipefd[2];
  xpipe (pipefd);
  pid_t child = xfork ();
  if (child == 0)
    {
      char c;
      xclose (pipefd[1]);
      /* Wait until the parent closes the write end of the pipe.  */
      TEST_COMPARE (read (pipefd[0], &c, 1), 0);
      _exit (0);
    }
  TEST_COMPARE (kcmp (pid, child, KCMP_FILE, fd1, fd1), 0);
  TEST_VERIFY (kcmp (pid, child, KCMP_VM, 0, 0) != 0);
  TEST_VERIFY (kcmp (pid, child, KCMP_FILES, 0, 0) != 0);

  /* Compare a file with the target file of an epoll set.  */
  int efd = epoll_create1 (EPOLL_CLOEXEC);
  TEST_VERIFY_EXIT (efd != -1);
  struct epoll_event ev = { .events = EPOLLIN };
  TEST_COMPARE (epoll_ctl (efd, EPOLL_CTL_ADD, pipefd[0], &ev), 0);
  struct kcmp_epoll_slot slot =
  {
    .efd = efd,
    .tfd = pipefd[0],
  };
  TEST_COMPARE (kcmp (pid, pid, KCMP_EPOLL_TFD, pipefd[0],
		      (uintptr_t) &slot), 0);
  TEST_VERIFY (kcmp (pid, pid, KCMP_EPOLL_TFD, fd1, (uintptr_t) &slot) != 0);

  TEST_COMPARE (kcmp (pid, pid, KCMP_FILE, fd1, -1), -1);
  TEST_COMPARE (errno, EBADF);
  TEST_COMPARE (kcmp (pid, pid, KCMP_TYPES, 0, 0), -1);
  TEST_COMPARE (errno, EINVAL);

  xclose (pipefd[1]);
  int status;
  xwaitpid (child, &status, 0);
  TEST_VERIFY (WIFEXITED (status) && WEXITSTATUS (status) == 0);

  /* The child no longer exists.  */
  TEST_COMPARE (kcmp (pid, child, KCMP_VM, 0, 0), -1);
  TEST_COMPARE (errno, ESRCH);

  xclose (efd);
  xclose (pipefd[0]);
  xclose (fd3);
  xclose (fd2);
  xclose (fd1);
  return 0;
}

#include <support/test-driver.c>
