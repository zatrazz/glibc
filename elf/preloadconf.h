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

#ifndef _PRELOADCONF_H
#define _PRELOADCONF_H

#include <stddef.h>
#include <stdint.h>

/* Read the preload list from FILENAME (relative to OPT_CHROOT if not
   NULL) and intern its names in the cache string table.  */
void parse_preloadconf (const char *filename, char *opt_chroot);

/* Return the contents of the cache_extension_tag_preload section, or
   NULL if there are no preload entries.  The string table indices are
   shifted by STRING_TABLE_OFFSET.  The section size is stored in
   *SIZE.  The caller must free the result.  */
uint32_t *get_preloadconf_ext (uint32_t string_table_offset, size_t *size);

#endif
