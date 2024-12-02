/* Test module for making nonexecutable stacks executable
   on load of a DSO that requires executable stacks.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void callme (void (*callback) (void));

/* This is a function that makes use of executable stack by
   using a local function trampoline.  */
void
tryme (void)
{
  bool ok = false;
  void callback (void) { ok = true; }

  callme (&callback);

  if (ok)
    printf ("DSO called ok (local %p, trampoline %p)\n", &ok, &callback);
  else
    abort ();
}

void
callme (void (*callback) (void))
{
  (*callback) ();
}
