/* Load a shared object at run time.
   Copyright (C) 1995-2023 Free Software Foundation, Inc.
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
#include <libintl.h>
#include <stddef.h>
#include <unistd.h>
#include <ldsodefs.h>
#include <errno.h>
#include <shlib-compat.h>

struct dlopen_args
{
  /* The arguments for dlopen_doit.  */
  const char *file;
  /* ELF header at offset in file.  */
  off_t offset;
  int mode;
  /* The return value of dlopen_doit.  */
  void *new;
  /* Address of the caller.  */
  const void *caller;
};


/* Non-shared code has no support for multiple namespaces.  */
#ifdef SHARED
# define NS __LM_ID_CALLER
#else
# define NS LM_ID_BASE
#endif


static void
dlopen_doit (void *a)
{
  struct dlopen_args *args = (struct dlopen_args *) a;

  if (args->mode & ~(RTLD_BINDING_MASK | RTLD_NOLOAD | RTLD_DEEPBIND
		     | RTLD_GLOBAL | RTLD_LOCAL | RTLD_NODELETE
		     | __RTLD_SPROF))
    _dl_signal_error (0, NULL, NULL, _("invalid mode parameter"));

  args->new = GLRO(dl_open) (args->file ?: "", args->offset, args->mode | __RTLD_DLOPEN,
			     args->caller,
			     args->file == NULL ? LM_ID_BASE : NS,
			     __libc_argc, __libc_argv, __environ);
}

static void *
dlopen_implementation (const char *file, int mode, off_t offset,  void *dl_caller)
{
  struct dlopen_args args;
  args.file = file;
  args.offset = offset;
  args.mode = mode;
  args.caller = dl_caller;

  return _dlerror_run (dlopen_doit, &args) ? NULL : args.new;
}

#ifdef SHARED
void *
___dlopen (const char *file, int mode)
{
  if (GLRO (dl_dlfcn_hook) != NULL)
    return GLRO (dl_dlfcn_hook)->dlopen (file, mode, RETURN_ADDRESS (0));
  else
    return dlopen_implementation (file, mode, 0, RETURN_ADDRESS (0));
}
versioned_symbol (libc, ___dlopen, dlopen, GLIBC_2_34);

# if OTHER_SHLIB_COMPAT (libdl, GLIBC_2_1, GLIBC_2_34)
compat_symbol (libdl, ___dlopen, dlopen, GLIBC_2_1);
# endif
#else /* !SHARED */
/* Also used with _dlfcn_hook.  */
void *
__dlopen (const char *file, int mode, void *dl_caller)
{
  return dlopen_implementation (file, mode, 0, dl_caller);
}

void *
___dlopen (const char *file, int mode)
{
  return __dlopen (file, mode, RETURN_ADDRESS (0));
}
weak_alias (___dlopen, dlopen)
static_link_warning (dlopen)
#endif /* !SHARED */


# ifdef SHARED
void *
___dlopen_with_offset (const char *file, off_t offset, int mode, void *dl_caller)
{
  if (GLRO (dl_dlfcn_hook) != NULL)
    return GLRO (dl_dlfcn_hook)->dlopen_with_offset (file, offset, mode, dl_caller);

  return dlopen_implementation (file, mode, offset, RETURN_ADDRESS (0));
}
versioned_symbol (libc, ___dlopen_with_offset, __google_dlopen_with_offset, GLIBC_2_15);

void *
___dlopen_with_offset64 (const char *file, off64_t offset, int mode, void *dl_caller)
{
#ifndef __OFF_T_MATCHES_OFF64_T
  if (offset > 0xFFFFFFFF) {
    _dl_signal_error(EFBIG, "__dlopen_with_offset64", NULL,
		     N_("File offset too large. Only 32 bit ELF supported."));
    return NULL;
  }
#endif
  return ___dlopen_with_offset(file, offset, mode, RETURN_ADDRESS (0));
}
versioned_symbol (libc, ___dlopen_with_offset64, __google_dlopen_with_offset64, GLIBC_2_15);

#else /* !SHARED */
/* Also used with _dlfcn_hook.  */
void *
__dlopen_with_offset (const char *file, off_t offset, int mode, void *dl_caller)
{
  return dlopen_implementation (file, mode, offset, RETURN_ADDRESS (0));
}
void *
___dlopen_with_offset (const char *file, off_t offset, int mode)
{
  return __dlopen_with_offset (file, offset, mode, RETURN_ADDRESS (0));
}
weak_alias (___dlopen_with_offset, __google_dlopen_with_offset)
static_link_warning (__google_dlopen_with_offset)

void *
__dlopen_with_offset64 (const char *file, off64_t offset, int mode, void *dl_caller)
{
#ifndef __OFF_T_MATCHES_OFF64_T
  if (offset > 0xFFFFFFFF) {
    _dl_signal_error(EFBIG, "__dlopen_with_offset64", NULL,
		     N_("File offset too large. Only 32 bit ELF supported."));
    return NULL;
  }
#endif
  return ___dlopen_with_offset(file, offset, mode);
}
void *
___dlopen_with_offset64 (const char *file, off64_t offset, int mode)
{
  return __dlopen_with_offset64(file, offset, mode, RETURN_ADDRESS (0));
}
weak_alias (___dlopen_with_offset64, __google_dlopen_with_offset64)
static_link_warning (__google_dlopen_with_offset64)
# endif /* !SHARED */
