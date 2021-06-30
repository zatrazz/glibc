/* Check if DT_AUDIT a module built with bind-now does trigger la_symbind.
   Copyright (C) 2021 Free Software Foundation, Inc.
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

#include <link.h>
#include <stdlib.h>
#include <string.h>

unsigned int
la_version (unsigned int version)
{
  return version;
}

unsigned int
la_objopen (struct link_map* map, Lmid_t lmid, uintptr_t* cookie)
{
  return LA_FLG_BINDFROM | LA_FLG_BINDTO;
}

uintptr_t
#if __ELF_NATIVE_CLASS == 32
lasymbind32 (Elf32_Sym *sym, unsigned int ndx, uintptr_t *refcook,
              uintptr_t *defcook, unsigned int *flags, const char *symname)
#else
la_symbind64 (Elf64_Sym *sym, unsigned int ndx, uintptr_t *refcook,
              uintptr_t *defcook, unsigned int *flags, const char *symname)
#endif
{
  /* Filter out glibc own symbols libc.so is not built with -z,now.  */
  if (strcmp (symname, "foo") == 0)
    exit (EXIT_SUCCESS);
  return sym->st_value;
} 
