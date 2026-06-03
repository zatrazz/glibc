/* Stress the __cxa_atexit/__cxa_finalize registration and recycle paths.
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

/* The exit handler list is stored in mmap'd pages that are kept
   read-only (PROT_READ) except while an entry is being registered or
   marked done.  This test exercises every write path on the list under
   churn: __cxa_atexit must briefly unfreeze a page to add an entry,
   __cxa_finalize must briefly unfreeze it to mark entries ef_free (and
   it also runs them), and a subsequent __cxa_atexit must recycle the
   now-free slots rather than grow the list without bound.  A missing
   unfreeze on any of these paths faults; this stress loop makes such a
   fault overwhelmingly likely to be hit.  The complementary read-only
   invariant is checked by tst-atexit-frozen.  */

#include <stdint.h>
#include <stdlib.h>
#include <support/check.h>

/* Public ABI entry points; not declared in any installed header.  */
extern int __cxa_atexit (void (*) (void *), void *, void *);
extern void __cxa_finalize (void *);

static int call_count;

static void
handler (void *arg)
{
  call_count += (int) (intptr_t) arg;
}

/* Address used as a distinct, non-NULL DSO handle so __cxa_finalize
   targets exactly the entries registered below.  */
static char dso_handle[1];

static int
do_test (void)
{
  enum { rounds = 200, per_round = 400 };

  for (int r = 0; r < rounds; r++)
    {
      /* per_round comfortably exceeds the number of entries that fit in
	 one page, so several blocks are allocated, frozen, finalized and
	 then recycled on the following round.  */
      call_count = 0;
      for (int i = 0; i < per_round; i++)
	TEST_COMPARE (__cxa_atexit (handler, (void *) (intptr_t) 1,
				    dso_handle), 0);

      /* Runs each handler exactly once and marks the slots ef_free.  */
      __cxa_finalize (dso_handle);
      TEST_COMPARE (call_count, per_round);

      /* A second finalize must be a no-op (all entries already free):
	 it must neither crash on the frozen pages nor re-run handlers.  */
      __cxa_finalize (dso_handle);
      TEST_COMPARE (call_count, per_round);
    }

  return 0;
}

#include <support/test-driver.c>
