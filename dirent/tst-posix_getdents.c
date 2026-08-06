/* Test posix_getdents.
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/check.h>
#include <support/support.h>
#include <support/temp_file.h>
#include <support/xunistd.h>

#define NFILES 100

static int
do_test (void)
{
  /* Create a directory with a known set of file names.  */
  char *dirname = support_create_temp_directory ("tst-posix_getdents.");
  for (int i = 0; i < NFILES; i++)
    {
      char *file = xasprintf ("%s/file%d", dirname, i);
      add_temp_file (file);
      xclose (xopen (file, O_WRONLY | O_CREAT | O_EXCL, 0600));
      free (file);
    }

  /* Read it back with posix_getdents, in a deliberately small buffer
     to exercise the record iteration across multiple calls.  */
  int fd = xopen (dirname, O_RDONLY | O_DIRECTORY, 0);
  bool found[NFILES] = { false };
  bool found_dot = false, found_dotdot = false;
  int nentries = 0;

  enum { buf_size = 3 * sizeof (struct posix_dent) + 3 * NAME_MAX };
  void *buf = xmalloc (buf_size);
  for (;;)
    {
      ssize_t ret = posix_getdents (fd, buf, buf_size, 0);
      TEST_VERIFY_EXIT (ret >= 0);
      if (ret == 0)
	break;

      for (char *p = buf; p < (char *) buf + ret; )
	{
	  struct posix_dent *dent = (struct posix_dent *) p;
	  TEST_VERIFY_EXIT (dent->d_reclen > 0);
	  TEST_VERIFY_EXIT (p + dent->d_reclen <= (char *) buf + ret);

	  nentries++;
	  if (strcmp (dent->d_name, ".") == 0)
	    {
	      found_dot = true;
	      TEST_VERIFY (dent->d_type == DT_DIR
			   || dent->d_type == DT_UNKNOWN);
	    }
	  else if (strcmp (dent->d_name, "..") == 0)
	    found_dotdot = true;
	  else
	    {
	      int i;
	      TEST_COMPARE (sscanf (dent->d_name, "file%d", &i), 1);
	      TEST_VERIFY_EXIT (i >= 0 && i < NFILES);
	      TEST_VERIFY (!found[i]);
	      found[i] = true;
	      TEST_VERIFY (dent->d_type == DT_REG
			   || dent->d_type == DT_UNKNOWN);
	      TEST_VERIFY (dent->d_ino != 0);
	    }

	  p += dent->d_reclen;
	}
    }

  TEST_VERIFY (found_dot);
  TEST_VERIFY (found_dotdot);
  TEST_COMPARE (nentries, NFILES + 2);
  for (int i = 0; i < NFILES; i++)
    TEST_VERIFY (found[i]);

  /* No flags are currently supported.  */
  errno = 0;
  TEST_COMPARE (posix_getdents (fd, buf, buf_size, 1), -1);
  TEST_COMPARE (errno, EINVAL);

  /* A too small buffer fails with EINVAL.  */
  xlseek (fd, 0, SEEK_SET);
  errno = 0;
  TEST_COMPARE (posix_getdents (fd, buf, 1, 0), -1);
  TEST_COMPARE (errno, EINVAL);

  /* Invalid file descriptor.  */
  errno = 0;
  TEST_COMPARE (posix_getdents (-1, buf, buf_size, 0), -1);
  TEST_COMPARE (errno, EBADF);

  /* Not a directory.  */
  int filefd = xopen ("/dev/null", O_RDONLY, 0);
  errno = 0;
  TEST_COMPARE (posix_getdents (filefd, buf, buf_size, 0), -1);
  TEST_COMPARE (errno, ENOTDIR);

  xclose (filefd);
  xclose (fd);
  free (buf);
  free (dirname);

  return 0;
}

#include <support/test-driver.c>
