/* Memory sealing.  Linux version.
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

#include <atomic.h>
#include <dl-mseal.h>
#include <dl-tunables.h>
#include <ldsodefs.h>

int
_dl_mseal (void *addr, size_t len)
{
  int r;
#if __ASSUME_MSEAL
  r = INTERNAL_SYSCALL_CALL (mseal, addr, len, 0);
#else
  r = -ENOSYS;
  static int mseal_supported = true;
  if (atomic_load_relaxed (&mseal_supported))
    {
      r = INTERNAL_SYSCALL_CALL (mseal, addr, len, 0);
      if (r == -ENOSYS)
	atomic_store_relaxed (&mseal_supported, false);
    }
#endif
  return r;
}
