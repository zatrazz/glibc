/* System-specific extensions of <sys/mman.h>, Linux version.
   Copyright (C) 2022-2026 Free Software Foundation, Inc.
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

#ifndef _SYS_MMAN_H
# error "Never include <bits/mman_ext.h> directly; use <sys/mman.h> instead."
#endif

#ifdef __USE_GNU
struct iovec;
extern __ssize_t process_madvise (int __pid_fd, const struct iovec *__iov,
				  size_t __count, int __advice,
				  unsigned __flags)
  __THROW;

extern int process_mrelease (int pidfd, unsigned int flags) __THROW;

# ifdef __has_include
#  if __has_include ("linux/mman.h")
#   include "linux/mman.h"
#  endif
# endif

# ifndef _LINUX_MMAN_H
/* Byte range of the file queried by cachestat.  A zero LEN means up to
   the end of the file.  */
struct cachestat_range
{
  __uint64_t off;
  __uint64_t len;
};

/* Page cache statistics returned by cachestat, in number of pages.  */
struct cachestat
{
  __uint64_t nr_cache;
  __uint64_t nr_dirty;
  __uint64_t nr_writeback;
  __uint64_t nr_evicted;
  __uint64_t nr_recently_evicted;
};
# endif

/* Kernel headers older than Linux 6.5 do not define them.  */
struct cachestat_range;
struct cachestat;

/* Store in *CSTAT the page cache statistics of the file FD in the byte
   range *CSTAT_RANGE.  FLAGS must be zero.  Return 0 on success, or -1
   and set errno on failure.  */
extern int cachestat (int __fd, const struct cachestat_range *__cstat_range,
		      struct cachestat *__cstat, unsigned int __flags)
  __THROW;

#endif /* __USE_GNU  */
