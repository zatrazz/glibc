/* Copyright (C) 2000-2025 Free Software Foundation, Inc.
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
#include <spawn_int.h>
#include <string.h>

_Static_assert (sizeof (posix_spawnattr_t) == sizeof (struct __spawn_attr),
		"sizeof (posix_spawnattr_t) != sizeof (struct __spawn_attr)");

/* Initialize data structure for file attribute for `spawn' call.  */
int
__posix_spawnattr_init (posix_spawnattr_t *attr)
{
  /* All elements have to be initialized to the default values which
     is generally zero.  */
  struct __spawn_attr *at = (struct __spawn_attr *) attr;
  memset (at, '\0', sizeof (*attr));

  return 0;
}
weak_alias (__posix_spawnattr_init, posix_spawnattr_init)
