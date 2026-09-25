/* Auditor for tst-audit-symbind-dlsym that requests all bindings.
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

#include <link.h>
#include <string.h>

unsigned int
la_version (unsigned int version)
{
  return LAV_CURRENT;
}

unsigned int
la_objopen (struct link_map *map, Lmid_t lmid, uintptr_t *cookie)
{
  return LA_FLG_BINDFROM | LA_FLG_BINDTO;
}

#if __ELF_NATIVE_CLASS == 64
uintptr_t
la_symbind64 (Elf64_Sym *sym, unsigned int ndx,
	      uintptr_t *refcook, uintptr_t *defcook,
	      unsigned int *flags, const char *symname)
#else
uintptr_t
la_symbind32 (Elf32_Sym *sym, unsigned int ndx,
	      uintptr_t *refcook, uintptr_t *defcook,
	      unsigned int *flags, const char *symname)
#endif
{
  if ((*flags & LA_SYMB_DLSYM) != 0
      && strcmp (symname, "tst_audit_symbind_dlsym_data") == 0)
    return sym->st_value + 1 * sizeof (int);
  return sym->st_value;
}
