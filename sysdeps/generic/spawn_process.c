/* Internal implementation of __spawn_process*.  Generic implementation.
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

#include <spawn.h>
#include <not-errno.h>

int
__spawn_process_create (process_create_id_t *procid,
			const char *path,
			const posix_spawn_file_actions_t *facts,
			const posix_spawnattr_t *attr,
			char *const argv[],
			char *const envp[])
{
  return __posix_spawn (procid, path, facts, attr, argv, envp);
}

int
__spawn_process_kill (process_create_id_t procid, int signo)
{
  return __kill (procid, signo);
}

process_create_id_t
__spawn_process_wait (process_create_id_t procid, int *wstatus, int options)
{
  return __waitpid (procid, wstatus, options | WEXITED);
}
