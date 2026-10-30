/* Basic tests for Linux statmount and listmount.
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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <support/check.h>
#include <support/namespace.h>
#include <support/support.h>
#include <support/xunistd.h>

_Static_assert (sizeof (struct mnt_id_req) == MNT_ID_REQ_SIZE_VER0
		|| sizeof (struct mnt_id_req) == MNT_ID_REQ_SIZE_VER1,
		"unexpected struct mnt_id_req size");

#define MAX_MOUNTS 1024
static uint64_t ids[MAX_MOUNTS];
static uint64_t ids_rev[MAX_MOUNTS];

/* Large enough for the three strings returned by the kernel.  */
#define BUFSIZE (64 * 1024)

static int
do_test (void)
{
  bool private_ns = false;
  if (support_become_root ())
    private_ns = support_enter_mount_namespace ();

  struct mnt_id_req req =
  {
    .size = sizeof (req),
    .mnt_id = LSMT_ROOT,
  };
  ssize_t n = listmount (&req, ids, MAX_MOUNTS, 0);
  if (n == -1 && errno == ENOSYS)
    FAIL_UNSUPPORTED ("listmount is not supported by the kernel");
  TEST_VERIFY_EXIT (n > 0);
  printf ("info: %zd mounts\n", n);

  /* The mount IDs are listed in increasing order.  */
  for (ssize_t i = 1; i < n; i++)
    TEST_VERIFY (ids[i - 1] < ids[i]);

  /* Restart the listing after the first ID.  */
  if (n > 1)
    {
      req.param = ids[0];
      TEST_COMPARE (listmount (&req, ids_rev, MAX_MOUNTS, 0), n - 1);
      TEST_COMPARE_BLOB (ids_rev, (n - 1) * sizeof (ids[0]),
			 &ids[1], (n - 1) * sizeof (ids[0]));
      req.param = 0;
    }

  /* The mount table might change if the test does not run in its own mount
     namespace.  */
  if (private_ns && n < MAX_MOUNTS)
    {
      TEST_COMPARE (listmount (&req, ids_rev, MAX_MOUNTS, LISTMOUNT_REVERSE),
		    n);
      for (ssize_t i = 0; i < n; i++)
	TEST_COMPARE (ids_rev[i], ids[n - 1 - i]);
    }

  TEST_COMPARE (listmount (&req, ids, MAX_MOUNTS, LISTMOUNT_REVERSE << 1),
		-1);
  TEST_COMPARE (errno, EINVAL);

  /* Check the root mount against statx.  */
  struct statx stx;
  TEST_COMPARE (statx (AT_FDCWD, "/", 0, STATX_MNT_ID_UNIQUE, &stx), 0);
  TEST_VERIFY_EXIT (stx.stx_mask & STATX_MNT_ID_UNIQUE);

  struct statmount *sm = xmalloc (BUFSIZE);
  req.mnt_id = stx.stx_mnt_id;
  req.param = STATMOUNT_SB_BASIC | STATMOUNT_MNT_BASIC | STATMOUNT_MNT_POINT
	      | STATMOUNT_FS_TYPE;
  TEST_COMPARE (statmount (&req, sm, BUFSIZE, 0), 0);
  TEST_VERIFY (sm->size <= BUFSIZE);
  TEST_COMPARE (sm->mask & req.param, req.param);
  TEST_COMPARE (sm->mnt_id, stx.stx_mnt_id);
  TEST_COMPARE (sm->sb_dev_major, stx.stx_dev_major);
  TEST_COMPARE (sm->sb_dev_minor, stx.stx_dev_minor);
  TEST_COMPARE_STRING (sm->str + sm->mnt_point, "/");
  TEST_VERIFY (sm->str[sm->fs_type] != '\0');
  printf ("info: / is a %s mount\n", sm->str + sm->fs_type);

  /* Every listed mount can be queried.  */
  req.param = STATMOUNT_MNT_BASIC;
  for (ssize_t i = 0; i < n; i++)
    {
      req.mnt_id = ids[i];
      if (statmount (&req, sm, BUFSIZE, 0) == 0)
	TEST_COMPARE (sm->mnt_id, ids[i]);
      else
	/* The mount might have been removed meanwhile.  */
	TEST_VERIFY (!private_ns && errno == ENOENT);
    }

  /* The mount ID must be valid.  */
  req.mnt_id = 0;
  TEST_COMPARE (statmount (&req, sm, BUFSIZE, 0), -1);
  TEST_COMPARE (errno, EINVAL);

  /* Query the mount by file descriptor.  The descriptor goes in the field
     after SIZE, which kernel headers older than Linux 7.0 do not call
     mnt_fd.  */
  {
    _Static_assert (offsetof (struct mnt_id_req, mnt_id)
		    == 2 * sizeof (uint32_t), "mnt_fd follows size");
    int fd = xopen ("/", O_PATH, 0);
    uint32_t mnt_fd = fd;
    req.mnt_id = 0;
    memcpy ((char *) &req + sizeof (req.size), &mnt_fd, sizeof (mnt_fd));
    if (statmount (&req, sm, BUFSIZE, STATMOUNT_BY_FD) == 0)
      TEST_COMPARE (sm->mnt_id, stx.stx_mnt_id);
    else
      {
	TEST_COMPARE (errno, EINVAL);
	puts ("info: STATMOUNT_BY_FD not supported by the kernel");
      }
    xclose (fd);
  }

  free (sm);
  return 0;
}

#include <support/test-driver.c>
