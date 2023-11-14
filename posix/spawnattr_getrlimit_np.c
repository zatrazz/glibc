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

int
__posix_spawnattr_getrlimit_np (posix_spawnattr_t *__restrict attr,
				int resource,
#if __RLIM_T_MATCHES_RLIM64_T
				struct rlimit *rlim
#else
				struct rlimit64 *rlim
#endif
				)
{
  struct __spawn_attr *at = (struct __spawn_attr *) attr;

  if (resource >= RLIM_NLIMITS || at->__nrlimits == 0)
    return EINVAL;

  struct __spawn_rlimit *limit = spawn_attr_find_rlimit (at, resource);
  if (limit == NULL)
    return EINVAL;

  rlim->rlim_cur = limit->rlim.rlim_cur;
  rlim->rlim_max = limit->rlim.rlim_max;

  return 0;
}
weak_alias (__posix_spawnattr_getrlimit_np, posix_spawnattr_getrlimit_np)
