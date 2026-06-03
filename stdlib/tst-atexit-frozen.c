/* Verify the exit handler list pages are kept read-only.
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

/* The function pointers registered with atexit/on_exit/__cxa_atexit are
   stored PTR_MANGLE'd in mmap'd pages that, as a hardening measure, are
   kept read-only (PROT_READ) outside the brief windows where an entry is
   being registered or run.  This freezing is what prevents an attacker
   with a write primitive from corrupting the stored handler pointers
   between registration and process exit.

   This is a whitebox test: it links statically so it can reach the
   internal (hidden) __exit_funcs list head, then checks via
   /proc/self/maps that the page holding the most recently registered
   block carries no write permission.  A missing __exit_funcs_protect on
   a registration path would leave the page writable and fail here; the
   converse failure (a missing unprotect) faults and is covered by
   tst-atexit-recycle.  */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <support/check.h>

/* Minimal mirror of struct exit_function_list from stdlib/exit.h: only the
   first member (the chain link) is accessed here, so the rest is omitted
   to avoid pulling in internal headers.  __exit_funcs is the hidden list
   head, reachable because this test is statically linked.  */
struct exit_function_list
{
  struct exit_function_list *next;
};
extern struct exit_function_list *__exit_funcs;
extern struct exit_function_list *__quick_exit_funcs;

/* Walk to the permanent tail (the static initial block) of a chain.  */
static struct exit_function_list *
chain_tail (struct exit_function_list *l)
{
  while (l->next != NULL)
    l = l->next;
  return l;
}

static void
handler (void)
{
}

static void
quick_handler (void)
{
}

/* Whether ADDR falls in a writable mapping according to /proc/self/maps.
   Fails the test if ADDR is not mapped at all.  */
static bool
addr_is_writable (uintptr_t addr)
{
  FILE *f = fopen ("/proc/self/maps", "re");
  TEST_VERIFY_EXIT (f != NULL);

  char *line = NULL;
  size_t len = 0;
  bool found = false;
  bool writable = false;
  while (getline (&line, &len, f) > 0)
    {
      uintptr_t start, end;
      char perms[8];
      if (sscanf (line, "%" SCNxPTR "-%" SCNxPTR " %7s", &start, &end, perms)
	  != 3)
	continue;
      if (addr >= start && addr < end)
	{
	  found = true;
	  writable = perms[1] == 'w';
	  break;
	}
    }
  free (line);
  fclose (f);

  TEST_VERIFY_EXIT (found);
  return writable;
}

static int
do_test (void)
{
  /* Register one handler of each flavor; all land in the __exit_funcs
     list and the head block must end up frozen.  */
  TEST_COMPARE (atexit (handler), 0);
  TEST_COMPARE (on_exit ((void (*) (int, void *)) handler, NULL), 0);

  TEST_VERIFY_EXIT (__exit_funcs != NULL);
  TEST_VERIFY (!addr_is_writable ((uintptr_t) __exit_funcs));

  /* Registering across page boundaries must keep succeeding even though
     the blocks are read-only: the registrar transiently unfreezes the
     target page and re-freezes it.  The burst (far more than one page of
     handlers) spills into mmap'd overflow blocks, so the head is now an
     overflow block; it must be read-only.  */
  for (int i = 0; i < 1000; i++)
    TEST_COMPARE (atexit (handler), 0);

  TEST_VERIFY (__exit_funcs->next != NULL);
  TEST_VERIFY (!addr_is_writable ((uintptr_t) __exit_funcs));

  /* The static initial block is the permanent tail of the chain; it must
     stay frozen too, even though the head is now an overflow block.  */
  struct exit_function_list *tail = chain_tail (__exit_funcs);
  TEST_VERIFY (!addr_is_writable ((uintptr_t) tail));

  /* The atexit and quick_exit initial blocks live together in one RELRO
     object (so a single mprotect covers both, whether or not the object
     straddles a page).  Register a quick_exit handler and confirm its
     initial (tail) block is adjacent to the atexit one and read-only.  */
  TEST_COMPARE (at_quick_exit (quick_handler), 0);
  struct exit_function_list *quick_tail = chain_tail (__quick_exit_funcs);

  intptr_t delta = (char *) quick_tail - (char *) tail;
  TEST_VERIFY (delta > 0 && delta < (intptr_t) sysconf (_SC_PAGESIZE));
  TEST_VERIFY (!addr_is_writable ((uintptr_t) quick_tail));

  return 0;
}

#include <support/test-driver.c>
