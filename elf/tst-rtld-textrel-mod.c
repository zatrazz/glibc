/* Shared object with a text relocation, used by tst-rtld-textrel.
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

#include <stdint.h>

/* Force a text relocation in the object.  */
static const uintptr_t
address __attribute__ ((used, section (".text"))) = (uintptr_t) &address;

uintptr_t
tst_rtld_textrel_value (void)
{
  /* Read the value from memory, so that the compiler does not fold it.  */
  return *(const volatile uintptr_t *) &address;
}

const void *
tst_rtld_textrel_location (void)
{
  return &address;
}
