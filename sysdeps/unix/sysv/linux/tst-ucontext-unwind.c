/* Unwinding from a signal delivered inside the context functions (BZ 34575).
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

/* A signal left pending while blocked is delivered when swapcontext or
   setcontext installs the target mask, i.e. with the PC inside them.
   The backtrace must be the complete chain of one of the contexts.
   The makecontext trampoline must be the outermost frame, both when
   the function it started unwinds and inside the trampoline itself.  */

#include "tst-ucontext-unwind-common.c"

#include <gnu/lib-names.h>
#include <support/xdlfcn.h>

/* Deliver one SIGUSR1 during the next switch.  */
static void
switch_with_pending_signal (enum switch_mode m)
{
  sigset_t set;
  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  mode = m;
  TEST_COMPARE (sigprocmask (SIG_BLOCK, &set, NULL), 0);
  xraise (SIGUSR1);
  tcu_run (1);
  TEST_COMPARE (sigprocmask (SIG_UNBLOCK, &set, NULL), 0);
}

/* Check from the samples:
   - Exactly one sample, where zero means the switch never unblocked the
     signal. Two would mean it was delivered somewhere else as well.
   - No bad frames.
   - No truncation (trunc == 0), stopping early is a bug.
   - A complete chain of one context.  */
static void
check_one (const char *what, int first)
{
  int trunc;
  TEST_COMPARE (nsamples, first + 1);
  if (nsamples == first + 1)
    {
      TEST_COMPARE (check_samples (what, first, first + 1, &trunc, true), 0);
      /* A pending signal hits the mask system call, where the chain is
	 still intact.  */
      TEST_COMPARE (trunc, 0);
      const char *which;
      classify (&samples[first], &which);
      TEST_VERIFY (strncmp (which, "old/", 4) == 0
		   || strncmp (which, "new/", 4) == 0);
    }
}


static ucontext_t ctx_tmain, ctx_tramp;
static void *tramp_ra;

void tcu_tramp_entry (void);
void tcu_run_tramp (void);
void tcu_tramp_probe (void);

static struct sample probe;

void __attribute_optimization_barrier__
tcu_tramp_probe (void)
{
  probe.pc = (uintptr_t) TCU_RETURN_ADDRESS ();
  probe.n = unwind_backtrace (probe.frames, NFRAMES);
}

void __attribute_optimization_barrier__
tcu_tramp_entry (void)
{
  tramp_ra = TCU_RETURN_ADDRESS ();
  tcu_tramp_probe ();
  /* Delivered by setcontext (uc_link) called from the trampoline.  */
  sigset_t set;
  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  TEST_COMPARE (sigprocmask (SIG_BLOCK, &set, NULL), 0);
  xraise (SIGUSR1);
}

void __attribute_optimization_barrier__
tcu_run_tramp (void)
{
  if (swapcontext (&ctx_tmain, &ctx_tramp) != 0)
    FAIL_EXIT1 ("swapcontext: %m");
}

/* The makecontext trampoline must end the backtrace: from the context
   function, from the setcontext (uc_link) the trampoline calls, and in the
   FDE lookup unwinders do for the return address minus one.  */
static void
check_trampoline (void)
{
  size_t stack_size = 256 * 1024;
  if (getcontext (&ctx_tramp) != 0)
    FAIL_EXIT1 ("getcontext: %m");
  ctx_tramp.uc_stack.ss_sp = xmalloc (stack_size);
  ctx_tramp.uc_stack.ss_size = stack_size;
  ctx_tramp.uc_link = &ctx_tmain;
  makecontext (&ctx_tramp, tcu_tramp_entry, 0);

  int first = nsamples;
  tcu_run_tramp ();
  sigset_t set;
  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  TEST_COMPARE (sigprocmask (SIG_UNBLOCK, &set, NULL), 0);

  /* From the started function: tcu_tramp_entry and then at most the
     trampoline.  */
  printf ("info: backtrace from the makecontext function:\n");
  print_sample (&probe);
  int i = 0;
  while (i < probe.n && (uintptr_t) probe.frames[i] != probe.pc)
    i++;
  TEST_VERIFY (i < probe.n);
  if (i < probe.n)
    {
      TEST_COMPARE_STRING (func_name ((uintptr_t) probe.frames[i] - 1),
			   "tcu_tramp_entry");
      TEST_VERIFY (probe.n - i <= 2);
      if (probe.n - i == 2)
	TEST_VERIFY (probe.frames[i + 1] == tramp_ra);
    }

  /* Inside the trampoline (setcontext on uc_link): only libc frames, or the
     resumed tcu_run_tramp.  */
  TEST_COMPARE (nsamples, first + 1);
  if (nsamples == first + 1)
    {
      const char *which;
      enum verdict v = classify (&samples[first], &which);
      printf ("info: trampoline: %s\n", which);
      print_sample (&samples[first]);
      TEST_VERIFY (v == V_TRUNC || strcmp (which, "tramp") == 0);
    }

  /* The unwinder looks up the FDE of a return address at the address minus
     one.  That must be the FDE of the return address itself (or no FDE for
     either), not whatever precedes the trampoline.  */
  void *h = xdlopen (LIBGCC_S_SO, RTLD_NOW);
  struct bases { void *tbase, *dbase, *func; } bases;
  const void *(*find_fde) (void *, struct bases *)
    = dlsym (h, "_Unwind_Find_FDE");
  if (find_fde != NULL)
    {
      const void *before = find_fde ((char *) tramp_ra - 1, &bases);
      const void *at = find_fde (tramp_ra, &bases);
      print_addr ("info: trampoline return address: ", (uintptr_t) tramp_ra);
      printf ("info: FDE of ra - 1: %p, FDE of ra: %p\n", before, at);
      TEST_VERIFY (before == at);
    }
  else
    printf ("info: _Unwind_Find_FDE not available\n");
  xdlclose (h);
}

static int
do_test (void)
{
  init_libc_base ();
  init_samples (16);
  install_handler (SIGUSR1, 0);
  init_contexts ();

  switch_with_pending_signal (MODE_SWAP);
  check_one ("swapcontext", 0);

  switch_with_pending_signal (MODE_SET);
  check_one ("setcontext", 1);

  check_trampoline ();
  return 0;
}

#include <support/test-driver.c>
