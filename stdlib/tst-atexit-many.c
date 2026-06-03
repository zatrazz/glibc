/* Verify LIFO ordering across the static block and mmap'd overflow blocks.
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

/* Registering well over one page of handlers forces the list to spill
   from the static fallback block into several mmap'd overflow blocks.
   This checks that the whole chain still runs in strict reverse
   registration order (POSIX LIFO), which exercises: freezing/unfreezing a
   page on every registration, allocating and freezing overflow blocks,
   traversal across the static/overflow boundary, and unmapping the
   overflow blocks (but not the static tail) as they drain at exit.  */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <support/check.h>

/* Public ABI entry point; not declared in any installed header.  */
extern int __cxa_atexit (void (*) (void *), void *, void *);

enum { nfns = 1000 };

static int order[nfns];
static int count;

static void
record (void *arg)
{
  int id = (int) (intptr_t) arg;
  if (count < nfns)
    order[count] = id;
  ++count;
}

/* Registered first, so it runs last: validate the recorded order and
   turn the result into the process exit status.  */
static void
verify (void *arg)
{
  if (count != nfns)
    _exit (1);
  for (int k = 0; k < nfns; k++)
    if (order[k] != nfns - 1 - k)
      _exit (2);
  _exit (0);
}

static int
do_test (void)
{
  TEST_COMPARE (__cxa_atexit (verify, NULL, NULL), 0);
  for (int i = 0; i < nfns; i++)
    TEST_COMPARE (__cxa_atexit (record, (void *) (intptr_t) i, NULL), 0);
  return 0;
}

#include <support/test-driver.c>
