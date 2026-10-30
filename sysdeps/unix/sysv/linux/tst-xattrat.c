/* Basic tests for Linux setxattrat, getxattrat, listxattrat, and
   removexattrat.
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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/xattr.h>
#include <support/check.h>
#include <support/support.h>
#include <support/temp_file.h>
#include <support/xunistd.h>

#define FILENAME "file"
#define NAME "user.glibc-test"

_Static_assert (sizeof (struct xattr_args) == 16,
		"sizeof (struct xattr_args) != 16");

static char *temp_dir;

static void
do_prepare (int argc, char *argv[])
{
  temp_dir = support_create_temp_directory ("tst-xattrat.");
  char *path = xasprintf ("%s/%s", temp_dir, FILENAME);
  add_temp_file (path);
  xclose (xopen (path, O_WRONLY | O_CREAT | O_EXCL, 0600));
  free (path);
}
#define PREPARE do_prepare

static ssize_t
get_value (int dfd, const char *path, unsigned int at_flags, char *buf,
	   size_t size)
{
  struct xattr_args args =
  {
    .value = (uintptr_t) buf,
    .size = size,
  };
  return getxattrat (dfd, path, at_flags, NAME, &args, sizeof (args));
}

static int
do_test (void)
{
  int dfd = xopen (temp_dir, O_RDONLY | O_DIRECTORY, 0);

  static const char value1[] = "value1";
  struct xattr_args args =
  {
    .value = (uintptr_t) value1,
    .size = sizeof (value1),
    .flags = XATTR_CREATE,
  };
  if (setxattrat (dfd, FILENAME, 0, NAME, &args, sizeof (args)) != 0)
    {
      if (errno == ENOSYS)
	FAIL_UNSUPPORTED ("setxattrat is not supported by the kernel");
      if (errno == EOPNOTSUPP)
	FAIL_UNSUPPORTED ("user extended attributes are not supported");
      FAIL_EXIT1 ("setxattrat: %m");
    }

  /* XATTR_CREATE fails if the attribute exists.  */
  TEST_COMPARE (setxattrat (dfd, FILENAME, 0, NAME, &args, sizeof (args)),
		-1);
  TEST_COMPARE (errno, EEXIST);

  char buf[64];
  TEST_COMPARE (get_value (dfd, FILENAME, 0, buf, sizeof (buf)),
		sizeof (value1));
  TEST_COMPARE_STRING (buf, value1);

  /* Replace the value, through a descriptor for the file itself.  */
  int fd = openat (dfd, FILENAME, O_RDONLY);
  TEST_VERIFY_EXIT (fd != -1);
  static const char value2[] = "value2-replaced";
  args.value = (uintptr_t) value2;
  args.size = sizeof (value2);
  args.flags = XATTR_REPLACE;
  TEST_COMPARE (setxattrat (fd, "", AT_EMPTY_PATH, NAME, &args,
			    sizeof (args)), 0);
  TEST_COMPARE (get_value (fd, NULL, AT_EMPTY_PATH, buf, sizeof (buf)),
		sizeof (value2));
  TEST_COMPARE_STRING (buf, value2);

  /* The buffer is too small.  */
  TEST_COMPARE (get_value (dfd, FILENAME, 0, buf, 1), -1);
  TEST_COMPARE (errno, ERANGE);

  /* getxattrat does not accept flags in struct xattr_args.  */
  args.value = (uintptr_t) buf;
  args.size = sizeof (buf);
  args.flags = XATTR_CREATE;
  TEST_COMPARE (getxattrat (dfd, FILENAME, 0, NAME, &args, sizeof (args)),
		-1);
  TEST_COMPARE (errno, EINVAL);

  /* The structure size must cover the first version.  */
  args.flags = 0;
  TEST_COMPARE (getxattrat (dfd, FILENAME, 0, NAME, &args,
			    sizeof (args) - 1), -1);
  TEST_COMPARE (errno, EINVAL);

  {
    /* With a zero size, listxattrat returns the size of the list.  */
    ssize_t size = listxattrat (dfd, FILENAME, 0, NULL, 0);
    TEST_VERIFY_EXIT (size > 0);
    char *list = xmalloc (size);
    ssize_t r = listxattrat (dfd, FILENAME, 0, list, size);
    TEST_COMPARE (r, size);
    bool found = false;
    for (char *p = list; p < list + r; p += strlen (p) + 1)
      found |= strcmp (p, NAME) == 0;
    TEST_VERIFY (found);
    free (list);
  }

  TEST_COMPARE (removexattrat (dfd, FILENAME, 0, NAME), 0);
  TEST_COMPARE (get_value (dfd, FILENAME, 0, buf, sizeof (buf)), -1);
  TEST_COMPARE (errno, ENODATA);
  TEST_COMPARE (removexattrat (fd, NULL, AT_EMPTY_PATH, NAME), -1);
  TEST_COMPARE (errno, ENODATA);

  /* Only AT_SYMLINK_NOFOLLOW and AT_EMPTY_PATH are accepted.  */
  TEST_COMPARE (removexattrat (dfd, FILENAME, AT_REMOVEDIR, NAME), -1);
  TEST_COMPARE (errno, EINVAL);

  xclose (fd);
  xclose (dfd);
  return 0;
}

#include <support/test-driver.c>
