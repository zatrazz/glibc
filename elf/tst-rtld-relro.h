/* Helpers to inspect the RELRO properties of loaded objects.
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
#include <stdbool.h>
#include <string.h>
#include <support/check.h>

struct relro_info
{
  const char *name;
  bool found;
  /* Whether the object has a PT_GNU_RELRO segment.  */
  bool relro;
  /* Whether the object was linked for immediate binding.  */
  bool now;
};

/* Set *RELRO and *NOW to the RELRO properties of the object INFO.  */
static void
get_relro_info (const struct dl_phdr_info *info, bool *relro, bool *now)
{
  *relro = false;
  *now = false;
  for (ElfW(Half) i = 0; i < info->dlpi_phnum; i++)
    {
      const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
      if (ph->p_type == PT_GNU_RELRO)
	*relro = true;
      else if (ph->p_type == PT_DYNAMIC)
	for (const ElfW(Dyn) *d
	       = (const ElfW(Dyn) *) (info->dlpi_addr + ph->p_vaddr);
	     d->d_tag != DT_NULL; d++)
	  if (d->d_tag == DT_BIND_NOW
	      || (d->d_tag == DT_FLAGS && (d->d_un.d_val & DF_BIND_NOW) != 0)
	      || (d->d_tag == DT_FLAGS_1 && (d->d_un.d_val & DF_1_NOW) != 0))
	    *now = true;
    }
}

static int
relro_info_callback (struct dl_phdr_info *info, size_t size, void *closure)
{
  struct relro_info *ri = closure;

  if (strcmp (info->dlpi_name, ri->name) != 0)
    return 0;

  ri->found = true;
  get_relro_info (info, &ri->relro, &ri->now);
  return 1;
}

/* Check that the object NAME has the expected RELRO properties.  */
static void
check_relro_info (const char *name, bool relro, bool now)
{
  struct relro_info ri = { .name = name };
  dl_iterate_phdr (relro_info_callback, &ri);
  TEST_VERIFY_EXIT (ri.found);
  TEST_COMPARE (ri.relro, relro);
  TEST_COMPARE (ri.now, now);
}
