/* Compile /etc/ld.so.preload into the ld.so.cache preload section.
   Copyright (C) 2026 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation; version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ldconfig.h>
#include <stringtable.h>

#include "preloadconf.h"

struct preload_entry
{
  struct preload_entry *next;
  struct stringtable_entry *name;
};

static struct preload_entry *entry_list;
static struct preload_entry **entry_list_next = &entry_list;
static uint32_t entry_count;

/* Each line holds names separated by white space or ':', as the
   dynamic loader used to accept in /etc/ld.so.preload.  Comments and
   'include' directives are handled by ldconfig_parse_config.  */
static void
add_preload_line (char *line, const char *filename, int lineno)
{
  if (line == NULL)
    return;

  char *name;
  while ((name = strsep (&line, " \t:")) != NULL)
    {
      if (name[0] == '\0')
	continue;

      struct preload_entry *entry = xmalloc (sizeof (*entry));
      entry->next = NULL;
      entry->name = cache_store_string (name);
      *entry_list_next = entry;
      entry_list_next = &entry->next;
      ++entry_count;
    }
}

void
parse_preloadconf (const char *filename, char *opt_chroot)
{
  ldconfig_parse_config (filename, opt_chroot, add_preload_line);
}

uint32_t *
get_preloadconf_ext (uint32_t string_table_offset, size_t *size)
{
  if (entry_count == 0)
    return NULL;

  *size = (entry_count + 1) * sizeof (uint32_t);
  uint32_t *data = xmalloc (*size);
  data[0] = entry_count;

  uint32_t i = 1;
  for (struct preload_entry *e = entry_list; e != NULL; e = e->next)
    data[i++] = string_table_offset + e->name->offset;

  return data;
}
