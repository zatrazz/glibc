/* Copyright (C) 2004-2023 Free Software Foundation, Inc.
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
#include <procutils.h>
#include <stdlib.h>
#include <stdint.h>

struct proc_self_find_map_t
{
  size_t size;
  uintptr_t ptr;
  uintptr_t ptr_end;
};

static int
proc_self_find_map (const char *line, void *arg)
{
  struct proc_self_find_map_t *a = arg;

  char *p;
  uintptr_t from = strtoul (line, &p, 16);
  if (p == line || *p++ != '-')
    return -1;

  char *q;
  uintptr_t to = strtoul (p, &q, 16);
  if (q == p || *q++ != ' ')
    return -1;

  if (from < a->ptr_end && to > a->ptr)
    {
      /* Found an entry that at least partially covers the area.  */
      if (*q++ != 'r' || *q++ != '-')
	return 1;

      if (from <= a->ptr && to >= a->ptr_end)
	{
	  a->size = 0;
	  return 1;
	}
      else if (from <= a->ptr)
	a->size -= to - a->ptr;
      else if (to >= a->ptr_end)
	a->size -= a->ptr_end - from;
      else
	a->size -= to - from;

      if (a->size == 0)
	return 1;
    }

  return 0;
}

int
__readonly_area (const char *ptr, size_t size)
{
  struct proc_self_find_map_t args =
    {
      .size = size,
      .ptr = (uintptr_t) ptr,
      .ptr_end = (uintptr_t) ptr + size
    };
  if (!__procutils_read_file ("/proc/self/maps", proc_self_find_map, &args))
    {
      /* It is the system administrator's choice to not have /proc
	 available to this process (e.g., because it runs in a chroot
	 environment.  Don't fail in this case.  */
      if (errno == ENOENT
	  /* The kernel has a bug in that a process is denied access
	     to the /proc filesystem if it is set[ug]id.  There has
	     been no willingness to change this in the kernel so
	     far.  */
	  || errno == EACCES
	  /* Process has reached the maximum number of open files.  */
	  || errno == EMFILE)
	return true;
      return false;
    }

  /* If the whole area between ptr and ptr_end is covered by read-only
     VMAs, return 1.  Otherwise return -1.  */
  return args.size == 0 ? 1 : -1;
}
