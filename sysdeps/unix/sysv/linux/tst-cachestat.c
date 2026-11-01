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
#include <unistd.h>
#include <support/check.h>
#include <support/support.h>
#include <support/temp_file.h>
#include <support/xunistd.h>

/* Kernel headers older than Linux 6.5 provide <linux/mman.h> without the
   cachestat structures, which <sys/mman.h> then only declares, so use
   copies of them.  */
struct test_cachestat_range
{
  uint64_t off;
  uint64_t len;
};

struct test_cachestat
{
  uint64_t nr_cache;
  uint64_t nr_dirty;
  uint64_t nr_writeback;
  uint64_t nr_evicted;
  uint64_t nr_recently_evicted;
};

static int
test_cachestat (int fd, const struct test_cachestat_range *range,
		struct test_cachestat *cs, unsigned int flags)
{
  return cachestat (fd, (const struct cachestat_range *) range,
		    (struct cachestat *) cs, flags);
}

/* Number of pages written to the file.  */
enum { npages = 4 };

static int fd;

static struct test_cachestat
query (uint64_t off, uint64_t len)
{
  struct test_cachestat_range range = { .off = off, .len = len };
  struct test_cachestat cs;
  /* Check that all the fields are written.  */
  memset (&cs, 0xff, sizeof (cs));
  if (test_cachestat (fd, &range, &cs, 0) != 0)
    FAIL_EXIT1 ("cachestat (%" PRIu64 ", %" PRIu64 "): %m", off, len);
  TEST_VERIFY (cs.nr_dirty <= cs.nr_cache);
  TEST_VERIFY (cs.nr_writeback <= cs.nr_cache);
  TEST_VERIFY (cs.nr_recently_evicted <= cs.nr_evicted);
  return cs;
}

static void
check_range (uint64_t off, uint64_t len, uint64_t expected)
{
  struct test_cachestat cs = query (off, len);
  if (cs.nr_cache + cs.nr_evicted != expected)
    FAIL ("cachestat (%" PRIu64 ", %" PRIu64 "): nr_cache %llu"
	  " + nr_evicted %llu != %" PRIu64, off, len,
	  (unsigned long long int) cs.nr_cache,
	  (unsigned long long int) cs.nr_evicted, expected);
}

static int
do_test (void)
{
  fd = create_temp_file ("tst-cachestat", NULL);
  TEST_VERIFY_EXIT (fd != -1);

  struct test_cachestat_range range = { 0 };
  struct test_cachestat cs;
  if (test_cachestat (fd, &range, &cs, 0) != 0)
    {
      if (errno == ENOSYS)
	FAIL_UNSUPPORTED ("cachestat is not supported by the kernel");
      FAIL_EXIT1 ("cachestat: %m");
    }
  /* The file is empty.  */
  TEST_COMPARE (cs.nr_cache, 0);
  TEST_COMPARE (cs.nr_evicted, 0);

  long int page_size = sysconf (_SC_PAGESIZE);
  size_t size = npages * page_size;
  char *buf = xmalloc (size);
  memset (buf, 0x55, size);
  xwrite (fd, buf, size);
  free (buf);

  /* The byte range is rounded to whole pages.  */
  check_range (0, size, npages);
  check_range (page_size, 2 * page_size, 2);
  check_range (page_size / 2, 1, 1);
  check_range (page_size / 2, page_size, 2);
  check_range ((npages - 1) * page_size, page_size, 1);

  /* A zero length means up to the end of the file, but a large folio may
     extend past it.  */
  cs = query (0, 0);
  TEST_VERIFY (cs.nr_cache + cs.nr_evicted >= npages);

  /* No page is dirty or under writeback after fsync.  */
  TEST_COMPARE (fsync (fd), 0);
  cs = query (0, size);
  TEST_COMPARE (cs.nr_dirty, 0);
  TEST_COMPARE (cs.nr_writeback, 0);
  TEST_COMPARE (cs.nr_cache + cs.nr_evicted, npages);

  /* FLAGS must be zero.  */
  range.len = size;
  TEST_COMPARE (test_cachestat (fd, &range, &cs, 1), -1);
  TEST_COMPARE (errno, EINVAL);

  TEST_COMPARE (test_cachestat (-1, &range, &cs, 0), -1);
  TEST_COMPARE (errno, EBADF);

  /* hugetlbfs is not supported.  */
  int hfd = memfd_create ("tst-cachestat", MFD_CLOEXEC | MFD_HUGETLB);
  if (hfd == -1)
    printf ("info: cannot create hugetlbfs file: %m\n");
  else
    {
      TEST_COMPARE (test_cachestat (hfd, &range, &cs, 0), -1);
      TEST_COMPARE (errno, EOPNOTSUPP);
      xclose (hfd);
    }

  xclose (fd);
  return 0;
}

#include <support/test-driver.c>
