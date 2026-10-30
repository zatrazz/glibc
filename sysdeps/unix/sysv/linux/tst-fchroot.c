/* Basic tests for Linux fchroot.
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
#include <stdio.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <support/check.h>
#include <support/namespace.h>
#include <support/support.h>
#include <support/temp_file.h>
#include <support/xunistd.h>

#define MARKER "fchroot-marker"

static char *temp_dir;
static char *marker_path;

static void
do_prepare (int argc, char *argv[])
{
  temp_dir = support_create_temp_directory ("tst-fchroot.");
  marker_path = xasprintf ("%s/%s", temp_dir, MARKER);
  add_temp_file (marker_path);
  xclose (xopen (marker_path, O_WRONLY | O_CREAT | O_EXCL, 0600));
}
#define PREPARE do_prepare

/* Move the root to failfs.  Unprivileged processes can do this if
   no_new_privs is set.  */
static void
change_root_failfs (void *closure)
{
  TEST_COMPARE (prctl (PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0), 0);
  if (fchroot (FD_FAILFS_ROOT, 0) != 0)
    {
      /* An unprivileged process which is already in a chroot cannot escape
	 it this way.  */
      TEST_COMPARE (errno, EPERM);
      puts ("info: cannot change the root to failfs");
      return;
    }

  /* Absolute file name lookups now fail.  */
  TEST_COMPARE (open ("/", O_RDONLY | O_DIRECTORY), -1);
  TEST_COMPARE (errno, EOPNOTSUPP);
}

/* Change the root to TEMP_DIR, which requires CAP_SYS_CHROOT.  */
static void
change_root (void *closure)
{
  int dfd = xopen (temp_dir, O_RDONLY | O_DIRECTORY, 0);
  TEST_COMPARE (fchroot (dfd, 0), 0);
  xclose (dfd);

  /* The marker file is now at the root.  */
  TEST_COMPARE (access ("/" MARKER, F_OK), 0);
}

static int
do_test (void)
{
  int dfd = xopen (temp_dir, O_RDONLY | O_DIRECTORY, 0);

  /* FLAGS must be zero.  */
  TEST_COMPARE (fchroot (dfd, 1), -1);
  if (errno == ENOSYS)
    FAIL_UNSUPPORTED ("fchroot is not supported by the kernel");
  TEST_COMPARE (errno, EINVAL);

  TEST_COMPARE (fchroot (-1, 0), -1);
  TEST_COMPARE (errno, EBADF);

  {
    int fd = xopen (marker_path, O_RDONLY, 0);
    TEST_COMPARE (fchroot (fd, 0), -1);
    TEST_COMPARE (errno, ENOTDIR);
    xclose (fd);
  }

  support_isolate_in_subprocess (change_root_failfs, NULL);

  support_become_root ();
  if (support_can_chroot ())
    support_isolate_in_subprocess (change_root, NULL);
  else
    {
      TEST_COMPARE (fchroot (dfd, 0), -1);
      TEST_COMPARE (errno, EPERM);
      puts ("info: cannot change the root to a directory");
    }

  xclose (dfd);
  return 0;
}

#include <support/test-driver.c>
