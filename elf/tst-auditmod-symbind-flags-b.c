/* Auditor for tst-audit-symbind-flags that logs the flags it gets.
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
#include <stdio.h>
#include <string.h>
#include <tst-audit.h>

#define PREFIX "tst_audit_symbind_flags_func"

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
  if (strncmp (symname, PREFIX, strlen (PREFIX)) == 0)
    fprintf (stderr, "b: la_symbind: %s %u\n", symname,
	     *flags & (LA_SYMB_NOPLTENTER | LA_SYMB_NOPLTEXIT));
  return sym->st_value;
}

ElfW(Addr)
pltenter (ElfW(Sym) *sym, unsigned int ndx, uintptr_t *refcook,
	  uintptr_t *defcook, La_regs *regs, unsigned int *flags,
	  const char *symname, long int *framesizep)
{
  if (strncmp (symname, PREFIX, strlen (PREFIX)) == 0)
    fprintf (stderr, "b: la_pltenter: %s\n", symname);
  return sym->st_value;
}
