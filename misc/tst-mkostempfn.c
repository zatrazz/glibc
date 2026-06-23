/* Tests for mkostempfn.
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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <support/check.h>
#include <support/temp_file.h>
#include <support/xdirent.h>
#include <support/xunistd.h>

/* Verify NAME is PREFIX, then N_RANDOM characters, then SUFFIX.  */
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

/* A creation callback that opens NAME relative to a directory descriptor,
   with O_CLOEXEC and an exclusive create -- the common use.  */
struct openat_ctx
{
  int dirfd;
  mode_t mode;
};

static int
try_openat (char *name, void *arg)
{
  struct openat_ctx *c = arg;
  return openat (c->dirfd, name,
		 O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, c->mode);
}

/* A callback that reports EEXIST a fixed number of times before creating
   the file, recording the names it is asked to try.  */
struct retry_ctx
{
  int dirfd;
  int fail;
  int calls;
  char names[8][64];
};

static int
try_retry (char *name, void *arg)
{
  struct retry_ctx *c = arg;
  if (c->calls < 8)
    strcpy (c->names[c->calls], name);
  c->calls++;
  if (c->fail > 0)
    {
      c->fail--;
      errno = EEXIST;
      return -1;
    }
  return openat (c->dirfd, name, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
}

/* A callback that always fails with a non-EEXIST error.  */
static int
try_eacces (char *name, void *arg)
{
  errno = EACCES;
  return -1;
}

/* A callback that creates a directory rather than a file, to show the
   interface is not tied to open.  */
static int
try_mkdir (char *name, void *arg)
{
  int dirfd = *(int *) arg;
  return mkdirat (dirfd, name, 0700);
}

static int
do_test (void)
{
  umask (0);
  char *dir = support_create_temp_directory ("tst-mkostempfn-");
  int dirfd = xopen (dir, O_RDONLY | O_DIRECTORY, 0);

  /* Common case: openat callback, name returned, mode honored, the name
     handed to the callback matches the one returned.  */
  {
    struct openat_ctx c = { dirfd, 0600 };
    char *name = NULL;
    int fd = mkostempfn ("base-", 0, ".txt", try_openat, &c, &name);
    TEST_VERIFY_EXIT (fd >= 0);
    TEST_VERIFY_EXIT (name != NULL);
    check_name (name, "base-", ".txt", 6);

    struct stat64 st;
    TEST_COMPARE (fstatat64 (dirfd, name, &st, 0), 0);
    TEST_VERIFY (S_ISREG (st.st_mode));
    TEST_COMPARE (st.st_mode & 07777, 0600);
    TEST_VERIFY (fcntl (fd, F_GETFD) & FD_CLOEXEC);
    xclose (fd);
    free (name);
  }

  /* NULL nameout: only the descriptor is wanted.  */
  {
    struct openat_ctx c = { dirfd, 0600 };
    int fd = mkostempfn ("anon-", 0, "", try_openat, &c, NULL);
    TEST_VERIFY_EXIT (fd >= 0);
    struct stat64 st;
    xfstat64 (fd, &st);
    TEST_VERIFY (S_ISREG (st.st_mode));
    xclose (fd);
  }

  /* NULL prefix and suffix default to empty; custom N_RANDOM length.  */
  {
    struct openat_ctx c = { dirfd, 0600 };
    char *name = NULL;
    int fd = mkostempfn (NULL, 16, NULL, try_openat, &c, &name);
    TEST_VERIFY_EXIT (fd >= 0);
    check_name (name, "", "", 16);
    xclose (fd);
    free (name);
  }

  /* EEXIST from the callback retries with a fresh name each time.  */
  {
    struct retry_ctx c = { .dirfd = dirfd, .fail = 3 };
    char *name = NULL;
    int fd = mkostempfn ("retry-", 0, "", try_retry, &c, &name);
    TEST_VERIFY_EXIT (fd >= 0);
    TEST_COMPARE (c.calls, 4);            /* 3 EEXIST + 1 success.  */
    check_name (name, "retry-", "", 6);
    /* The successful name is the last one the callback saw.  */
    TEST_COMPARE_STRING (name, c.names[3]);
    /* Each attempt used a different name.  */
    for (int i = 0; i < c.calls; i++)
      for (int j = i + 1; j < c.calls; j++)
	TEST_VERIFY (strcmp (c.names[i], c.names[j]) != 0);
    xclose (fd);
    free (name);
  }

  /* A non-EEXIST failure aborts immediately and propagates errno; nameout
     is not touched.  */
  {
    char *name = (char *) -1;
    errno = 0;
    TEST_COMPARE (mkostempfn ("x-", 0, "", try_eacces, NULL, &name), -1);
    TEST_COMPARE (errno, EACCES);
    TEST_VERIFY (name == (char *) -1);
  }

  /* The callback need not create a regular file: here it makes a
     directory, and the non-negative return is propagated.  */
  {
    char *name = NULL;
    int r = mkostempfn ("dir-", 0, "", try_mkdir, &dirfd, &name);
    TEST_VERIFY_EXIT (r == 0);
    check_name (name, "dir-", "", 6);
    struct stat64 st;
    TEST_COMPARE (fstatat64 (dirfd, name, &st, 0), 0);
    TEST_VERIFY (S_ISDIR (st.st_mode));
    free (name);
  }

  /* Remove everything we created so the temporary directory can be
     cleaned up.  */
  DIR *dirp = xopendir (dir);
  struct dirent *e;
  while ((e = readdir (dirp)) != NULL)
    if (strcmp (e->d_name, ".") != 0 && strcmp (e->d_name, "..") != 0)
      {
	if (unlinkat (dirfd, e->d_name, 0) != 0 && errno == EISDIR)
	  TEST_COMPARE (unlinkat (dirfd, e->d_name, AT_REMOVEDIR), 0);
      }
  xclosedir (dirp);

  xclose (dirfd);
  free (dir);
  return 0;
}

#include <support/test-driver.c>
