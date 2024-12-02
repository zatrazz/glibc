/* Test program for executable stacks in an executable itself.

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

#include <string.h>

#include "tst-execstack-mod.c"	/* This defines the `tryme' test function.  */

static void deeper (void (*f) (void));

static int
do_test (void)
{
  tryme ();

  /* Test that growing the stack region gets new executable pages too.  */
  deeper (&tryme);

  return 0;
}

static void
deeper (void (*f) (void))
{
  char stack[1100 * 1024];
  explicit_bzero (stack, sizeof stack);
  (*f) ();
  memfrob (stack, sizeof stack);
}

#include <support/test-driver.c>
