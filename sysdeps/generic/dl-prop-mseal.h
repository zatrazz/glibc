/* Support for GNU properties.  Generic version.
   Copyright (C) 2024 Free Software Foundation, Inc.
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

#ifndef _DL_PROP_MSEAL_H
#define _LD_PROP_MSEAL_H

#include <dl-tunables.h>
#include <dl-mseal-mode.h>

static __always_inline bool
_dl_process_gnu_property_seal (struct link_map *l, int fd, uint32_t type,
			       uint32_t datasz, void *data)
{
  if (type == GNU_PROPERTY_NO_MEMORY_SEAL && datasz == 0)
    {
      int32_t mode = TUNABLE_GET (glibc, rtld, seal, int32_t, NULL);
      l->l_seal = (mode == DL_SEAL_ENFORCE) ? lt_seal_toseal : lt_seal_dont;
      return true;
    }
  return false;
}

#endif
