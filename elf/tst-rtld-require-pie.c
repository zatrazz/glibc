/* Test that glibc.rtld.require_pie allows position-independent executables.
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

#include <elf.h>
#include <link.h>
#include <support/check.h>

#ifndef EXPECTED_TYPE
# define EXPECTED_TYPE ET_DYN
#endif

/* Defined by the linker to point to the ELF header of the program.  */
extern const ElfW(Ehdr) __ehdr_start __attribute__ ((visibility ("hidden")));

static int
do_test (void)
{
  /* Reaching this point means the loader accepted the program, so only check
     that it was linked as expected.  */
  TEST_COMPARE (__ehdr_start.e_type, EXPECTED_TYPE);
  return 0;
}

#include <support/test-driver.c>
