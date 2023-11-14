/* Implement posix_spawn extension to setup resource limits.
   Copyright (C) 2025 Free Software Foundation, Inc.
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
#include <spawn.h>
#include <spawn_int.h>
#include <stdlib.h>

/* Initialize data structure for file attribute for `spawn' call.  */
int
__posix_spawnattr_setrlimit_np (posix_spawnattr_t *__restrict attr,
				int resource,
#if __RLIM_T_MATCHES_RLIM64_T
				const struct rlimit *rlim
#else
				const struct rlimit64 *rlim
#endif
				)
{
  if (resource < 0 || resource >= RLIM_NLIMITS)
    return EINVAL;

  struct __spawn_attr *at = (struct __spawn_attr *) attr;

  if (at->__rlimits == NULL)
    {
      at->__rlimits = __libc_reallocarray (at->__rlimits, RLIM_NLIMITS,
					      sizeof (struct rlimit64));
      if (at->__rlimits == NULL)
	return ENOMEM;
    }

  at->__rlimitset |= 1u << resource;
  at->__rlimits[resource].rlim_cur = rlim->rlim_cur;
  at->__rlimits[resource].rlim_max = rlim->rlim_max;

  return 0;
}
weak_alias (__posix_spawnattr_setrlimit_np, posix_spawnattr_setrlimit_np)
