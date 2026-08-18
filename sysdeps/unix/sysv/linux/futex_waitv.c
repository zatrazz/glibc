/* Wait on multiple futexes.  Linux implementation.
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
#include <stdint.h>
#include <time.h>
#include <sys/futex.h>
#include <sysdep.h>
#include <lowlevellock-futex.h>

/* The layout the futex_waitv system call expects (struct futex_waitv
   from <linux/futex.h>), along with the associated flag constants.  */
struct kernel_futex_waitv
{
  uint64_t val;
  uint64_t uaddr;
  uint32_t flags;
  uint32_t reserved;
};
#define FUTEX2_SIZE_U32		0x02

/* The exported symbol always operates on the 64-bit time_t layout,
   also on ABIs where the default time_t is 32 bits (the installed
   header only declares it for 64-bit time_t).  */
int
futex_waitv (const struct futex_waiter *waiters, unsigned int nwaiters,
	     clockid_t clockid, const struct __timespec64 *abstime,
	     unsigned int *index)
{
  if (nwaiters == 0 || nwaiters > FUTEX_WAITV_MAX)
    return EINVAL;

  /* Work around the fact that the kernel rejects negative timeout values
     despite them being valid absolute timeouts.  */
  if (__glibc_unlikely (abstime != NULL && abstime->tv_sec < 0))
    return ETIMEDOUT;

  struct kernel_futex_waitv kwaiters[FUTEX_WAITV_MAX];
  for (unsigned int i = 0; i < nwaiters; i++)
    {
      if ((waiters[i].flags & ~FUTEX_FLAG_SHARED) != 0)
	return EINVAL;
      kwaiters[i] = (struct kernel_futex_waitv)
	{
	  .val = waiters[i].expected,
	  .uaddr = (uintptr_t) waiters[i].addr,
	  .flags = FUTEX2_SIZE_U32
		   | ((waiters[i].flags & FUTEX_FLAG_SHARED)
		      ? 0 : FUTEX_PRIVATE_FLAG),
	};
    }

  /* The syscall only supports 64-bit time_t and always takes an absolute
     timeout measured against CLOCKID.  On success it returns the array
     index of one of the woken futexes.  */
  long int r = INTERNAL_SYSCALL_CALL (futex_waitv, kwaiters, nwaiters, 0,
				      abstime, clockid);
  if (r < 0)
    return -r;
  if (index != NULL)
    *index = r;
  return 0;
}
