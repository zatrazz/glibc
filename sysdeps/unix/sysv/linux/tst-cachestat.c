/* Basic tests for Linux cachestat.
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
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <support/check.h>
#include <support/support.h>
#include <support/temp_file.h>
#include <support/xunistd.h>

/* Kernel headers older than Linux 6.5 provide <linux/mman.h> without the
   cachestat structures, which <sys/mman.h> then only declares.  They were
   added along with the system call, so complete the types in that case.  */
#if defined _LINUX_MMAN_H && !defined __NR_cachestat
struct cachestat_range
{
  uint64_t off;
  uint64_t len;
};

struct cachestat
{
  uint64_t nr_cache;
  uint64_t nr_dirty;
  uint64_t nr_writeback;
  uint64_t nr_evicted;
  uint64_t nr_recently_evicted;
};
#endif

/* Number of pages written to the files.  */
enum { npages = 4 };

static long int page_size;

static struct cachestat
query (int fd, uint64_t off, uint64_t len)
{
  struct cachestat_range range = { .off = off, .len = len };
  struct cachestat cs;
  /* Check that all the fields are written.  */
  memset (&cs, 0xff, sizeof (cs));
  if (cachestat (fd, &range, &cs, 0) != 0)
    FAIL_EXIT1 ("cachestat (%" PRIu64 ", %" PRIu64 "): %m", off, len);
  TEST_VERIFY (cs.nr_dirty <= cs.nr_cache);
  TEST_VERIFY (cs.nr_writeback <= cs.nr_cache);
  TEST_VERIFY (cs.nr_recently_evicted <= cs.nr_evicted);
  return cs;
}

static void
check_range (int fd, uint64_t off, uint64_t len, uint64_t expected)
{
  struct cachestat cs = query (fd, off, len);
  if (cs.nr_cache + cs.nr_evicted != expected)
    FAIL ("cachestat (%" PRIu64 ", %" PRIu64 "): nr_cache %llu"
	  " + nr_evicted %llu != %" PRIu64, off, len,
	  (unsigned long long int) cs.nr_cache,
	  (unsigned long long int) cs.nr_evicted, expected);
}

/* Write NPAGES pages to FD.  */
static void
write_pages (int fd)
{
  size_t size = npages * page_size;
  char *buf = xmalloc (size);
  memset (buf, 0x55, size);
  xwrite (fd, buf, size);
  free (buf);
}

static int
do_test (void)
{
  page_size = sysconf (_SC_PAGESIZE);
  size_t size = npages * page_size;

  /* The cachestat reports the page cache of the file itself, so the counts
     depend on the file system.  A memfd file is on tmpfs, which keeps its
     pages there (or in swap, reported as evicted).  On overlayfs, for
     instance, the data is cached by the underlying file and cachestat reports
     no page.  */
  int fd = memfd_create ("tst-cachestat", MFD_CLOEXEC);
  if (fd == -1)
    FAIL_EXIT1 ("memfd_create: %m");

  struct cachestat_range range = { 0 };
  struct cachestat cs;
  if (cachestat (fd, &range, &cs, 0) != 0)
    {
      if (errno == ENOSYS)
	FAIL_UNSUPPORTED ("cachestat is not supported by the kernel");
      /* The kernel only fails with EPERM for files the caller cannot write
	 to, which is not the case of FD, so it comes from a seccomp filter
	 (such as the default podman one).  */
      if (errno == EPERM)
	FAIL_UNSUPPORTED ("cachestat is not permitted: %m");
      FAIL_EXIT1 ("cachestat: %m");
    }
  TEST_COMPARE (cs.nr_cache, 0);
  TEST_COMPARE (cs.nr_evicted, 0);

  write_pages (fd);

  /* The byte range is rounded to whole pages.  */
  check_range (fd, 0, size, npages);
  check_range (fd, page_size, 2 * page_size, 2);
  check_range (fd, page_size / 2, 1, 1);
  check_range (fd, page_size / 2, page_size, 2);
  check_range (fd, (npages - 1) * page_size, page_size, 1);

  /* A zero length means up to the end of the file, but a large folio may
     extend past it.  */
  cs = query (fd, 0, 0);
  TEST_VERIFY (cs.nr_cache + cs.nr_evicted >= npages);

  /* FLAGS must be zero.  */
  range.len = size;
  TEST_COMPARE (cachestat (fd, &range, &cs, 1), -1);
  TEST_COMPARE (errno, EINVAL);

  TEST_COMPARE (cachestat (-1, &range, &cs, 0), -1);
  TEST_COMPARE (errno, EBADF);

  xclose (fd);

  /* No page is dirty or under writeback after fsync.  fsync does not
     write back tmpfs pages, so use a file in TMPDIR.  */
  fd = create_temp_file ("tst-cachestat", NULL);
  TEST_VERIFY_EXIT (fd != -1);
  write_pages (fd);
  TEST_COMPARE (fsync (fd), 0);
  cs = query (fd, 0, size);
  if (cs.nr_cache + cs.nr_evicted == 0)
    printf ("info: no page is reported for a file in TMPDIR (overlayfs?),"
	    " skipping fsync check\n");
  else
    {
      TEST_COMPARE (cs.nr_dirty, 0);
      TEST_COMPARE (cs.nr_writeback, 0);
      TEST_COMPARE (cs.nr_cache + cs.nr_evicted, npages);
    }
  xclose (fd);

  /* hugetlbfs is not supported.  */
  int hfd = memfd_create ("tst-cachestat", MFD_CLOEXEC | MFD_HUGETLB);
  if (hfd == -1)
    printf ("info: cannot create hugetlbfs file: %m\n");
  else
    {
      TEST_COMPARE (cachestat (hfd, &range, &cs, 0), -1);
      TEST_COMPARE (errno, EOPNOTSUPP);
      xclose (hfd);
    }

  return 0;
}

#include <support/test-driver.c>
