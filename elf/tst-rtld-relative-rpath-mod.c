/* Shared object with relative RUNPATH entries for tst-rtld-relative-rpath.
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

#include <dlfcn.h>

int tst_rtld_relative_rpath_dummy;

/* dlopen uses the search path of the calling object.  */
void *
tst_rtld_relative_rpath_open (const char *name)
{
  void *h = dlopen (name, RTLD_NOW);

  /* Ensure this is not a tail call, otherwise the caller of this function
     would be used.  */
  ++tst_rtld_relative_rpath_dummy;
  return h;
}
