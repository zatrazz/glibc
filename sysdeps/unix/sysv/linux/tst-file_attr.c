/* Basic tests for Linux file_getattr and file_setattr.
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
#include <stdlib.h>
#include <sys/file_attr.h>
#include <support/check.h>
#include <support/temp_file.h>
#include <support/xunistd.h>

_Static_assert (sizeof (struct file_attr) == FILE_ATTR_SIZE_VER0,
		"sizeof (struct file_attr) != FILE_ATTR_SIZE_VER0");

static int
do_test (void)
{
  char *path;
  int fd = create_temp_file ("tst-file_attr.", &path);
  TEST_VERIFY_EXIT (fd != -1);

  struct file_attr fa;
  if (file_getattr (AT_FDCWD, path, &fa, sizeof (fa), 0) != 0)
    {
      if (errno == ENOSYS)
	FAIL_UNSUPPORTED ("file_getattr is not supported by the kernel");
      if (errno == EOPNOTSUPP)
	FAIL_UNSUPPORTED ("file attributes are not supported");
      FAIL_EXIT1 ("file_getattr: %m");
    }

  /* The same attributes through the file descriptor.  */
  struct file_attr fa2;
  TEST_COMPARE (file_getattr (fd, NULL, &fa2, sizeof (fa2), AT_EMPTY_PATH),
		0);
  TEST_COMPARE (fa2.fa_xflags, fa.fa_xflags);

  /* The structure size must cover the first version.  */
  TEST_COMPARE (file_getattr (AT_FDCWD, path, &fa2, FILE_ATTR_SIZE_VER0 - 1,
			      0), -1);
  TEST_COMPARE (errno, EINVAL);

  /* Only AT_SYMLINK_NOFOLLOW and AT_EMPTY_PATH are accepted.  */
  TEST_COMPARE (file_getattr (AT_FDCWD, path, &fa2, sizeof (fa2),
			      AT_REMOVEDIR), -1);
  TEST_COMPARE (errno, EINVAL);

  /* The owner of the file can set FS_XFLAG_NODUMP.  */
  fa.fa_xflags |= FS_XFLAG_NODUMP;
  if (file_setattr (AT_FDCWD, path, &fa, sizeof (fa), 0) == 0)
    {
      TEST_COMPARE (file_getattr (fd, "", &fa2, sizeof (fa2), AT_EMPTY_PATH),
		    0);
      TEST_VERIFY (fa2.fa_xflags & FS_XFLAG_NODUMP);

      fa.fa_xflags &= ~FS_XFLAG_NODUMP;
      TEST_COMPARE (file_setattr (fd, NULL, &fa, sizeof (fa), AT_EMPTY_PATH),
		    0);
      TEST_COMPARE (file_getattr (AT_FDCWD, path, &fa2, sizeof (fa2), 0), 0);
      TEST_VERIFY (!(fa2.fa_xflags & FS_XFLAG_NODUMP));
    }
  else
    {
      TEST_COMPARE (errno, EOPNOTSUPP);
      puts ("info: FS_XFLAG_NODUMP is not supported by the file system");
    }

  xclose (fd);
  free (path);
  return 0;
}

#include <support/test-driver.c>
