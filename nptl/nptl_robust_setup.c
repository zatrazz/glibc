/* Linux robust mutext setup.
   Copyright (C) 2025 Free Software Foundation, Inc.
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

#include <descr.h>
#include <atomic.h>
#include <pthreadP.h>

int __nptl_set_robust_list_avail = 1;

bool
__nptl_robust_setup (struct robust_list_head *robust_head)
{
  if (atomic_load_relaxed (&__nptl_set_robust_list_avail))
    {
      int res = INTERNAL_SYSCALL_CALL (set_robust_list, robust_head,
				       sizeof (struct robust_list_head));
      if (!INTERNAL_SYSCALL_ERROR_P (res))
        return true;

      atomic_store_relaxed (&__nptl_set_robust_list_avail, 0);
    }
  return false;
}
