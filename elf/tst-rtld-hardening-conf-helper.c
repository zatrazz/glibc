/* Helper program for tst-rtld-hardening-conf.
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

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>

#ifdef TEXTREL
/* Force a text relocation in the program.  */
static const uintptr_t
address __attribute__ ((used, section (".text"))) = (uintptr_t) &address;
#endif

#ifdef WX_SEGMENT
/* The linker places a section that is both writable and executable in a
   writable and executable PT_LOAD segment.  */
__asm__ (".section .tst_rtld_wx,\"awx\",%progbits\n"
	 ".byte 0\n"
	 ".previous");
#endif

/* Load the shared object given as the argument, if any, with lazy
   binding.  */
int
main (int argc, char *argv[])
{
#ifdef TEXTREL
  /* Read the value from memory, so that the compiler does not fold it.  */
  if (*(const volatile uintptr_t *) &address != (uintptr_t) &address)
    {
      puts ("error: text relocation not applied");
      return 2;
    }
#endif

  if (argc > 1 && dlopen (argv[1], RTLD_LAZY) == NULL)
    {
      printf ("error: %s\n", dlerror ());
      return 1;
    }

  puts ("ok");
  return 0;
}
