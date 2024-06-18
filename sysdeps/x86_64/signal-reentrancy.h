/* Handle Linux signal reentracy.
   Copyright (C) 2022 Free Software Foundation, Inc.
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

#ifndef _SIGNAL_REENTRACY_H
#define _SIGNAL_REENTRACY_H

static inline bool
signal_exchange_value (void **mem, void **v)
{
  uintptr_t addr = 1UL;
  asm volatile ("xaddq %1, %0"
		: "+m" (*mem), "+r" (addr)
		: : "memory");
  if (addr & 1UL)
    {
      *mem = (void *) addr;
      return false;
    }
  *v = (void *) addr;
  return true;
}

static inline void
signal_store_value (void **mem, void *value)
{
  *(volatile void **) mem = value;
}

#endif
