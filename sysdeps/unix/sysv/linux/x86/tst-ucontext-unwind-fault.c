/* Unwinding after the stack switch in swapcontext/setcontext (BZ 34575).
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

/* swapcontext and setcontext push the resume address on the target
   stack after switching to it.  Write-protecting that stack slot makes
   the push fault, so the SIGSEGV backtrace starts after the switch and
   must be the complete chain of the target context.  */

#include "tst-ucontext-unwind-common.c"

#include <support/xunistd.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __x86_64__
# define REG_SP REG_RSP
#else
# define REG_SP REG_ESP
#endif

static void *protected_page;
static size_t page_size;

static void
segv_handler (int sig, siginfo_t *si, void *ctx)
{
  if ((uintptr_t) si->si_addr - (uintptr_t) protected_page >= page_size)
    {
      /* Not the expected fault: crash.  */
      signal (SIGSEGV, SIG_DFL);
      return;
    }
  sample_handler (sig, si, ctx);
  mprotect (protected_page, page_size, PROT_READ | PROT_WRITE);
}

static void
check_mode (enum switch_mode m, const char *what, const char *chain)
{
  /* The resume address goes just below the saved stack pointer.  */
  uintptr_t slot = ctx_new.uc_mcontext.gregs[REG_SP] - sizeof (void *);
  protected_page = (void *) (slot & -page_size);
  int first = nsamples;
  mode = m;
  xmprotect (protected_page, page_size, PROT_READ);
  tcu_run (1);

  TEST_COMPARE (nsamples, first + 1);
  if (nsamples == first + 1)
    {
      const char *which;
      enum verdict v = classify (&samples[first], &which);
      printf ("info: %s: %s\n", what, which);
      print_sample (&samples[first]);
      TEST_VERIFY (v == V_CHAIN);
      TEST_COMPARE_STRING (which, chain);
    }
}

static int
do_test (void)
{
  page_size = xsysconf (_SC_PAGESIZE);
  init_libc_base ();
  init_samples (4);
  install_handler (SIGUSR1, 0);
  struct sigaction sa = { .sa_sigaction = segv_handler,
			  .sa_flags = SA_SIGINFO | SA_ONSTACK };
  sigemptyset (&sa.sa_mask);
  xsigaction (SIGSEGV, &sa, NULL);
  init_contexts ();

  /* The new context was parked by swapcontext, then by getcontext.  */
  check_mode (MODE_SWAP, "swapcontext", "new/swap");
  check_mode (MODE_SET, "setcontext", "new/swap");
  check_mode (MODE_SET, "setcontext", "new/set");
  return 0;
}

#include <support/test-driver.c>
