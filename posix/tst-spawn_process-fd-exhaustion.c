/* Check if __spawn_process_create works if there is no available
   file descriptor.
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
#include <intprops.h>
#include <paths.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/resource.h>

#include <support/check.h>
#include <support/temp_file.h>
#include <support/xunistd.h>

#include <stdio.h>

static const char pidfile[] = OBJPFX "tst-spawn_process-fd-exhaustion.pid";

static int
do_test (void)
{
  struct rlimit rl;
  int max_fd = 24;

  if (getrlimit (RLIMIT_NOFILE, &rl) == -1)
    FAIL_EXIT1 ("getrlimit (RLIMIT_NOFILE): %m");

  max_fd = (rl.rlim_cur < max_fd ? rl.rlim_cur : max_fd);
  rl.rlim_cur = max_fd;

  if (setrlimit (RLIMIT_NOFILE, &rl) == -1)
    FAIL_EXIT1 ("setrlimit (RLIMIT_NOFILE): %m");

  /* Exhauste the file descriptor limit with temporary files.  */
  int files[max_fd];
  int nfiles = 0;
  for (; nfiles < max_fd; nfiles++)
    {
      int fd = create_temp_file ("tst-spawn_process-fd-exhaustion.pid.", NULL);
      if (fd == -1)
	{
	  if (errno != EMFILE)
	    FAIL_EXIT1 ("create_temp_file: %m");
	  break;
	}
      int flags = fcntl (fd, F_GETFD, 0);
      TEST_VERIFY_EXIT (flags != -1);
      TEST_VERIFY_EXIT (fcntl (fd, F_SETFD, flags | FD_CLOEXEC) != -1);
      files[nfiles] = fd;
    }
  TEST_VERIFY_EXIT (nfiles != 0);

  process_create_id_t pid;
  {
    posix_spawn_file_actions_t fa;
    TEST_COMPARE (posix_spawn_file_actions_init (&fa), 0);
    TEST_COMPARE (posix_spawn_file_actions_addopen (&fa, STDOUT_FILENO,
						    pidfile,
						    O_WRONLY| O_CREAT
						    | O_TRUNC,
						    0644), 0);

    TEST_COMPARE (posix_spawn_file_actions_adddup2 (&fa, STDOUT_FILENO,
						    STDERR_FILENO), 0);
    char *spawn_argv[] =
      {
	(char *) _PATH_BSHELL,
	(char *) "-c",
	(char *) "echo $$",
	NULL
      };
    int r = __spawn_process_create (&pid, _PATH_BSHELL, &fa, NULL,
				    spawn_argv, NULL);
    TEST_COMPARE (r, 0);

    int status;
    TEST_COMPARE (__spawn_process_wait (pid, &status, 0), pid);
    TEST_COMPARE (WIFEXITED (status), 1);
    TEST_COMPARE (WEXITSTATUS (status), 0);
  }

  for (int i=0; i<nfiles; i++)
    xclose (files[i]);

  {
    int pidfd = xopen (pidfile, O_RDONLY, 0);

    char buf[INT_BUFSIZE_BOUND (pid_t)];
    ssize_t n = read (pidfd, buf, sizeof (buf));
    TEST_VERIFY (n < sizeof buf && n >= 0);

    /* We only expect to read the PID.  */
    char *endp;
    long int rpid = strtol (buf, &endp, 10);
    TEST_VERIFY (*endp == '\n' && endp != buf);

    TEST_COMPARE (rpid, pid);
  }

  return 0;
}

#include <support/test-driver.c>
