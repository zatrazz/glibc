/* Tests for mkostempat.
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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <support/check.h>
#include <support/temp_file.h>
#include <support/xdirent.h>
#include <support/xunistd.h>

/* Verify that the leaf name NAME has PREFIX as a prefix, SUFFIX as a
   suffix, and exactly N_RANDOM characters in between.  */
static void
check_name (const char *name, const char *prefix, const char *suffix,
	    unsigned int n_random)
{
  size_t prefixlen = strlen (prefix);
  size_t suffixlen = strlen (suffix);
  TEST_COMPARE (strlen (name), prefixlen + n_random + suffixlen);
  TEST_VERIFY (strncmp (name, prefix, prefixlen) == 0);
  TEST_VERIFY (strcmp (name + prefixlen + n_random, suffix) == 0);
}

static int
do_test (void)
{
  /* Make the recorded permission bits deterministic.  */
  umask (0);

  char *dir = support_create_temp_directory ("tst-mkostempat-");
  int dirfd = xopen (dir, O_RDONLY | O_DIRECTORY, 0);

  /* Basic case: default entropy, the name is allocated and returned, the
     descriptor is a regular file open for reading and writing, the
     requested mode is honored, and O_CLOEXEC is set by default.  */
  {
    char *name = NULL;
    int fd = mkostempat (dirfd, "base-", ".txt", 0, 0, 0600, &name);
    TEST_VERIFY_EXIT (fd >= 0);
    TEST_VERIFY_EXIT (name != NULL);
    check_name (name, "base-", ".txt", 6);

    /* The file is created relative to dirfd, not the cwd.  */
    struct stat64 st;
    xfstatat64 (dirfd, name, &st, 0);
    TEST_VERIFY (S_ISREG (st.st_mode));
    TEST_COMPARE (st.st_mode & 07777, 0600);

    TEST_COMPARE (write (fd, "hello", 5), 5);
    TEST_VERIFY (fcntl (fd, F_GETFD) & FD_CLOEXEC);

    xclose (fd);
    free (name);
  }

  /* Null prefix and suffix default to the empty string: the name is just
     the random characters.  */
  {
    char *name = NULL;
    int fd = mkostempat (dirfd, NULL, NULL, 0, 0, 0600, &name);
    TEST_VERIFY_EXIT (fd >= 0);
    TEST_VERIFY_EXIT (name != NULL);
    check_name (name, "", "", 6);
    struct stat64 st;
    xfstatat64 (dirfd, name, &st, 0);
    TEST_VERIFY (S_ISREG (st.st_mode));
    xclose (fd);
    free (name);
  }

  /* Custom entropy length and an empty suffix.  */
  {
    char *name = NULL;
    int fd = mkostempat (dirfd, "p", "", 12, 0, 0644, &name);
    TEST_VERIFY_EXIT (fd >= 0);
    check_name (name, "p", "", 12);
    struct stat64 st;
    xfstatat64 (dirfd, name, &st, 0);
    TEST_COMPARE (st.st_mode & 07777, 0644);
    TEST_VERIFY (fcntl (fd, F_GETFD) & FD_CLOEXEC);
    xclose (fd);
    free (name);
  }

  /* A large N_RANDOM, well above the default of six and above the number
     of base-62 digits produced by a single random draw (so the random
     buffer is refilled several times), yields exactly that many random
     characters, each drawn from the temporary-name alphabet [0-9A-Za-z].  */
  {
    enum { many = 40 };
    char *name = NULL;
    int fd = mkostempat (dirfd, "many-", ".dat", many, 0, 0600, &name);
    TEST_VERIFY_EXIT (fd >= 0);
    check_name (name, "many-", ".dat", many);
    const char *random = name + strlen ("many-");
    for (int i = 0; i < many; i++)
      TEST_VERIFY (isalnum ((unsigned char) random[i]));
    struct stat64 st;
    xfstatat64 (dirfd, name, &st, 0);
    TEST_VERIFY (S_ISREG (st.st_mode));
    xclose (fd);
    free (name);
  }

  /* Null namebuf: only the descriptor is wanted.  The file is still
     created (and unlinkable through /proc is not portable, so just check
     the descriptor itself).  */
  {
    int fd = mkostempat (dirfd, "anon-", "", 0, 0, 0600, NULL);
    TEST_VERIFY_EXIT (fd >= 0);
    struct stat64 st;
    xfstat64 (fd, &st);
    TEST_VERIFY (S_ISREG (st.st_mode));
    TEST_COMPARE (st.st_nlink, 1);
    TEST_VERIFY (fcntl (fd, F_GETFD) & FD_CLOEXEC);
    xclose (fd);
  }

  /* AT_FDCWD works like the other *at functions.  */
  {
    xchdir (dir);
    char *name = NULL;
    int fd = mkostempat (AT_FDCWD, "cwd-", ".tmp", 0, 0, 0600, &name);
    TEST_VERIFY_EXIT (fd >= 0);
    check_name (name, "cwd-", ".tmp", 6);
    TEST_COMPARE (access (name, F_OK), 0);
    TEST_VERIFY (fcntl (fd, F_GETFD) & FD_CLOEXEC);
    xclose (fd);
    free (name);
  }

  /* oflags are honored: request O_APPEND.  */
  {
    char *name = NULL;
    int fd = mkostempat (dirfd, "app-", "", 0, O_APPEND, 0600, &name);
    TEST_VERIFY_EXIT (fd >= 0);
    int fl = fcntl (fd, F_GETFL);
    TEST_VERIFY (fl & O_APPEND);
    TEST_VERIFY (fcntl (fd, F_GETFD) & FD_CLOEXEC);
    xclose (fd);
    free (name);
  }

  /* A '/' in either the prefix or the suffix is rejected with EINVAL: the
     generated name must be a single path component.  */
  {
    char *name = NULL;
    errno = 0;
    TEST_COMPARE (mkostempat (dirfd, "a/b", "", 0, 0, 0600, &name), -1);
    TEST_COMPARE (errno, EINVAL);

    errno = 0;
    TEST_COMPARE (mkostempat (dirfd, "ok", "x/y", 0, 0, 0600, &name), -1);
    TEST_COMPARE (errno, EINVAL);
  }

#ifdef O_TMPFILE
  /* O_TMPFILE requests an unnamed file and is incompatible with creating a
     named one, so it is rejected with EINVAL.  */
  {
    char *name = NULL;
    errno = 0;
    TEST_COMPARE (mkostempat (dirfd, "tmpf-", "", 0, O_TMPFILE, 0600, &name),
		  -1);
    TEST_COMPARE (errno, EINVAL);
  }
#endif

  /* A nonzero N_RANDOM smaller than three is rejected with EINVAL, while
     exactly three is accepted.  */
  {
    char *name = NULL;
    for (unsigned int n = 1; n <= 2; n++)
      {
	errno = 0;
	TEST_COMPARE (mkostempat (dirfd, "low-", "", n, 0, 0600, &name), -1);
	TEST_COMPARE (errno, EINVAL);
      }

    int fd = mkostempat (dirfd, "low-", "", 3, 0, 0600, &name);
    TEST_VERIFY_EXIT (fd >= 0);
    check_name (name, "low-", "", 3);
    xclose (fd);
    free (name);
  }

  /* An invalid directory descriptor surfaces the underlying error.  */
  {
    char *name = NULL;
    errno = 0;
    TEST_COMPARE (mkostempat (-1, "base-", "", 0, 0, 0600, &name), -1);
    TEST_COMPARE (errno, EBADF);
  }

  /* Remove every file we created so the temporary directory itself can be
     cleaned up on exit.  */
  DIR *dirp = xopendir (dir);
  struct dirent *e;
  while ((e = readdir (dirp)) != NULL)
    if (strcmp (e->d_name, ".") != 0 && strcmp (e->d_name, "..") != 0)
      TEST_COMPARE (unlinkat (dirfd, e->d_name, 0), 0);
  xclosedir (dirp);

  xclose (dirfd);
  free (dir);
  return 0;
}

#include <support/test-driver.c>
