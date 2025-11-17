/* Close a handle opened by `dlopen'.
   Copyright (C) 1995-2025 Free Software Foundation, Inc.
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
#include <ldsodefs.h>
#include <shlib-compat.h>

struct dlclose_args
{
  void *handle;
  void *caller;
};

static void
dlclose_doit (void *a)
{
  struct dlclose_args *args = (struct dlclose_args *) a;
  GLRO(dl_close) (args->handle, args->caller);
}

static int
dlclose_implementation (void *handle, void *dl_caller)
{
  return _dlerror_run (dlclose_doit, &(struct dlclose_args) {
				       .handle= handle,
				       .caller = RETURN_ADDRESS (0)}) ? -1 : 0;
}

#ifdef SHARED
int
___dlclose (void *handle)
{
  if (GLRO (dl_dlfcn_hook) != NULL)
    return GLRO (dl_dlfcn_hook)->dlclose (handle, RETURN_ADDRESS (0));
  return dlclose_implementation (handle, RETURN_ADDRESS (0));
}
versioned_symbol (libc, ___dlclose, dlclose, GLIBC_2_34);
#else
int
__dlclose (void *handle, void *dl_caller)
{
  return dlclose_implementation (handle, dl_caller);
}

int
___dlclose (void *handle)
{
  return __dlclose (handle, RETURN_ADDRESS (0));
}
weak_alias (___dlclose, dlclose)
#endif

#if OTHER_SHLIB_COMPAT (libdl, GLIBC_2_0, GLIBC_2_34)
compat_symbol (libdl, ___dlclose, dlclose, GLIBC_2_0);
#endif
