/* Basic tests for Linux listns.
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

#include <array_length.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/nsfs.h>
#include <support/check.h>
#include <support/namespace.h>
#include <support/support.h>
#include <support/xunistd.h>

/* Added in Linux 6.18, so it is available whenever listns is.  */
#ifndef NS_GET_ID
# define NS_GET_ID _IOR (0xb7, 13, uint64_t)
#endif

static const struct
{
  const char *name;
  uint32_t type;
} ns_types[] =
{
  { "cgroup", CLONE_NEWCGROUP },
  { "ipc", CLONE_NEWIPC },
  { "mnt", CLONE_NEWNS },
  { "net", CLONE_NEWNET },
  { "pid", CLONE_NEWPID },
  { "time", CLONE_NEWTIME },
  { "user", CLONE_NEWUSER },
  { "uts", CLONE_NEWUTS },
};

/* Return the ID of the namespace NAME of the calling process, or 0 if the
   kernel does not support this namespace type.  */
static uint64_t
get_ns_id (const char *name)
{
  char *path = xasprintf ("/proc/self/ns/%s", name);
  int fd = open (path, O_RDONLY | O_CLOEXEC);
  if (fd == -1)
    {
      if (errno != ENOENT)
	FAIL_EXIT1 ("open (\"%s\"): %m", path);
      free (path);
      return 0;
    }
  uint64_t id;
  if (ioctl (fd, NS_GET_ID, &id) != 0)
    FAIL_EXIT1 ("ioctl (\"%s\", NS_GET_ID): %m", path);
  xclose (fd);
  free (path);
  return id;
}

struct id_list
{
  uint64_t *ids;
  size_t count;
};

/* List all the namespaces of types TYPE owned by USER_NS_ID, with calls
   that return at most PAGE IDs.  listns cannot report how many namespaces
   there are (a zero NR_NS_IDS only returns zero), and namespaces can be
   created or destroyed between calls, so the IDs are retrieved in pages:
   each call continues after the last listed ID, set in REQ.NS_ID.  */
static struct id_list
list_all (uint32_t type, uint64_t user_ns_id, size_t page)
{
  struct ns_id_req req =
  {
    .size = NS_ID_REQ_SIZE_VER0,
    .ns_type = type,
    .user_ns_id = user_ns_id,
  };
  struct id_list l = { NULL, 0 };
  while (true)
    {
      l.ids = xrealloc (l.ids, (l.count + page) * sizeof (uint64_t));
      ssize_t r = listns (&req, l.ids + l.count, page, 0);
      /* If the previous page ended with the last namespace, there is no
	 namespace after REQ.NS_ID and listns fails with ENOENT instead of
	 returning zero.  */
      if (r == -1 && errno == ENOENT && req.ns_id != 0)
	break;
      if (r == -1)
	FAIL_EXIT1 ("listns (ns_type=%#" PRIx32 ", user_ns_id=%#" PRIx64
		    "): %m", type, user_ns_id);
      TEST_VERIFY_EXIT (r <= page);

      /* The IDs are listed in ascending order, after REQ.NS_ID.  */
      for (size_t i = 0; i < r; i++)
	{
	  TEST_VERIFY (l.ids[l.count + i] > req.ns_id);
	  req.ns_id = l.ids[l.count + i];
	}
      l.count += r;
      /* A partial page is the last one.  */
      if (r < page)
	break;
    }
  return l;
}

static bool
contains (const struct id_list *l, uint64_t id)
{
  for (size_t i = 0; i < l->count; i++)
    if (l->ids[i] == id)
      return true;
  return false;
}

/* Check the listing of the namespaces owned by a new user namespace,
   which are only the ones created along with it.  */
static void
check_owned (void *closure)
{
  if (unshare (CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWUTS) != 0)
    {
      printf ("info: cannot create user namespace: %m\n");
      return;
    }

  uint64_t mntns = get_ns_id ("mnt");
  uint64_t utsns = get_ns_id ("uts");
  uint64_t expected[] =
  {
    mntns < utsns ? mntns : utsns,
    mntns < utsns ? utsns : mntns,
  };

  const uint64_t users[] = { LISTNS_CURRENT_USER, get_ns_id ("user") };
  for (int i = 0; i < array_length (users); i++)
    {
      /* Also check that each call continues after the last listed ID.
	 With the two expected namespaces, pages of 1 and 2 IDs end the
	 listing with ENOENT, and a page of 3 IDs with a partial page.  */
      for (size_t page = 1; page <= 3; page++)
	{
	  struct id_list l = list_all (0, users[i], page);
	  TEST_COMPARE (l.count, array_length (expected));
	  if (l.count == array_length (expected))
	    TEST_COMPARE_BLOB (l.ids, l.count * sizeof (uint64_t),
			       expected, sizeof (expected));
	  free (l.ids);
	}

      struct id_list l = list_all (CLONE_NEWUTS, users[i], 4);
      TEST_COMPARE (l.count, 1);
      if (l.count == 1)
	TEST_COMPARE (l.ids[0], utsns);
      free (l.ids);
    }
}

static int
do_test (void)
{
  support_need_proc ("needs /proc/self/ns");

  struct ns_id_req req = { .size = NS_ID_REQ_SIZE_VER0 };
  uint64_t ids[1];
  if (listns (&req, ids, array_length (ids), 0) == -1 && errno == ENOSYS)
    FAIL_UNSUPPORTED ("listns is not supported by the kernel");

  uint64_t own_ids[array_length (ns_types)];
  for (size_t i = 0; i < array_length (ns_types); i++)
    own_ids[i] = get_ns_id (ns_types[i].name);

  /* The namespaces of the calling process are always listed, and only
     they are listed when selecting their type.  */
  struct id_list all = list_all (0, 0, 256);
  for (size_t i = 0; i < array_length (ns_types); i++)
    {
      if (own_ids[i] == 0)
	continue;
      if (!contains (&all, own_ids[i]))
	FAIL ("%s namespace %" PRIu64 " is not listed", ns_types[i].name,
	      own_ids[i]);

      struct id_list l = list_all (ns_types[i].type, 0, 256);
      for (size_t j = 0; j < array_length (ns_types); j++)
	if (own_ids[j] != 0 && contains (&l, own_ids[j]) != (i == j))
	  FAIL ("%s namespace %" PRIu64 " %s listed for type %s",
		ns_types[j].name, own_ids[j], i == j ? "is not" : "is",
		ns_types[i].name);
      free (l.ids);
    }
  free (all.ids);

  /* FLAGS must be zero.  */
  TEST_COMPARE (listns (&req, ids, array_length (ids), 1), -1);
  TEST_COMPARE (errno, EINVAL);

  /* The first published structure is the smallest supported one.  */
  req.size = NS_ID_REQ_SIZE_VER0 - 1;
  TEST_COMPARE (listns (&req, ids, array_length (ids), 0), -1);
  TEST_COMPARE (errno, EINVAL);
  req.size = NS_ID_REQ_SIZE_VER0;

  /* The reserved field must be zero.  */
  req.spare = 1;
  TEST_COMPARE (listns (&req, ids, array_length (ids), 0), -1);
  TEST_COMPARE (errno, EINVAL);
  req.spare = 0;

  /* Only namespace types can be selected.  */
  req.ns_type = CLONE_NEWUTS | CLONE_VM;
  TEST_COMPARE (listns (&req, ids, array_length (ids), 0), -1);
  TEST_COMPARE (errno, EOPNOTSUPP);

  support_isolate_in_subprocess (check_owned, NULL);

  return 0;
}

#include <support/test-driver.c>
