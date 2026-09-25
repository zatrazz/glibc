/* Common code for the ucontext unwinding tests (BZ 34575).
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

/* Two contexts, "old" (the test stack) and "new" (a makecontext stack),
   switch back and forth through known call chains, either with
   swapcontext or with getcontext plus setcontext.  A signal handler
   records a backtrace.  Every backtrace must be the complete chain of
   one of the two contexts, or stop inside libc.  The test functions are
   global so dladdr can name them (the tests link with -rdynamic).  The
   functions calling the context functions find their CFA from the stack
   pointer, their callers from the frame pointer (alloca), so both the
   stack pointer and the callee-saved registers must be right.  */

#include <dlfcn.h>
#include <inttypes.h>
#include <link.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/auxv.h>
#include <ucontext.h>
#include <unwind.h>
#include <unwind-arch.h>
#include <sigcontextinfo.h>
#include <support/check.h>
#include <support/support.h>
#include <support/xsignal.h>

#define NFRAMES 32

struct sample
{
  uintptr_t pc;
  int n;
  void *frames[NFRAMES];
};

static struct sample *samples;
static volatile sig_atomic_t nsamples;
static int maxsamples;

enum switch_mode { MODE_SWAP, MODE_SET };
static volatile enum switch_mode mode;
static volatile bool stop;

static ucontext_t ctx_old, ctx_new;

/* The address the function returns to, as the unwinder reports it.  */
#ifdef __arm__
/* Without the Thumb bit.  */
# define TCU_RETURN_ADDRESS() \
  ((void *) ((uintptr_t) __builtin_return_address (0) & ~(uintptr_t) 1))
#else
# define TCU_RETURN_ADDRESS() \
  __builtin_extract_return_addr (__builtin_return_address (0))
#endif

#if !UNWIND_LINK_FRAME_ADJUSTMENT
static inline void *
unwind_arch_adjustment (void *prev, void *addr)
{
  return addr;
}
#endif

/* The libgcc unwinder, without the frame pointer fallback backtrace
   has on some architectures.  */
struct unwind_arg
{
  void **frames;
  int n, size;
};

static _Unwind_Reason_Code
unwind_collect (struct _Unwind_Context *ctx, void *a)
{
  struct unwind_arg *arg = a;
  if (arg->n == arg->size)
    return _URC_END_OF_STACK;
  void *ip = (void *) _Unwind_GetIP (ctx);
  if (arg->n > 0)
    ip = unwind_arch_adjustment (arg->frames[arg->n - 1], ip);
  arg->frames[arg->n++] = ip;
  return _URC_NO_REASON;
}

static int
unwind_backtrace (void **frames, int size)
{
  struct unwind_arg arg = { frames, 0, size };
  _Unwind_Backtrace (unwind_collect, &arg);
  /* An undefined return address ends the chain with a zero.  */
  if (arg.n > 1 && frames[arg.n - 1] == NULL)
    arg.n--;
  return arg.n;
}

/* Force a frame pointer.  */
static volatile size_t alloca_size = 16;
#define USE_FRAME_POINTER() \
  do { char *p = __builtin_alloca (alloca_size); asm ("" :: "r" (p)); } \
  while (0)

/* Record the interrupted PC and a backtrace.  */
static void
sample_handler (int sig, siginfo_t *si, void *ctx)
{
  if (nsamples >= maxsamples)
    return;
  struct sample *s = &samples[nsamples];
  s->pc = sigcontext_get_pc (ctx);
  s->n = unwind_backtrace (s->frames, NFRAMES);
  nsamples = nsamples + 1;
}

void tcu_switch (ucontext_t *, ucontext_t *);
void tcu_old1 (void);
void tcu_old2 (void);
void tcu_old3 (void);
void tcu_run (unsigned int);
void tcu_new1 (void);
void tcu_new2 (void);
void tcu_new3 (void);
void tcu_new_entry (void);

/* swapcontext built from getcontext and setcontext.  */
void __attribute__ ((noipa))
tcu_switch (ucontext_t *save, ucontext_t *to)
{
  volatile bool back = false;
  if (getcontext (save) != 0)
    FAIL_EXIT1 ("getcontext: %m");
  if (!back)
    {
      back = true;
      setcontext (to);
      FAIL_EXIT1 ("setcontext: %m");
    }
}

/* A macro, swapcontext may be returns_twice and cannot be inlined.  */
#define do_switch(save, to)					\
  do								\
    {								\
      if (mode == MODE_SWAP)					\
	{							\
	  if (swapcontext (save, to) != 0)			\
	    FAIL_EXIT1 ("swapcontext: %m");			\
	}							\
      else							\
	tcu_switch (save, to);					\
    }								\
  while (0)

void __attribute__ ((noipa))
tcu_old1 (void)
{
  do_switch (&ctx_old, &ctx_new);
}

void __attribute__ ((noipa))
tcu_old2 (void)
{
  USE_FRAME_POINTER ();
  tcu_old1 ();
}

void __attribute__ ((noipa))
tcu_old3 (void)
{
  tcu_old2 ();
}

/* Switch to the new context N times (or until STOP).  */
void __attribute__ ((noipa))
tcu_run (unsigned int n)
{
  while (n-- > 0 && !stop)
    tcu_old3 ();
}

void __attribute__ ((noipa))
tcu_new1 (void)
{
  do_switch (&ctx_new, &ctx_old);
}

void __attribute__ ((noipa))
tcu_new2 (void)
{
  USE_FRAME_POINTER ();
  tcu_new1 ();
}

void __attribute__ ((noipa))
tcu_new3 (void)
{
  tcu_new2 ();
}

void __attribute__ ((noipa))
tcu_new_entry (void)
{
  while (true)
    tcu_new3 ();
}

/* Create the new context and park it inside tcu_new1.  */
static void
init_contexts (void)
{
  size_t stack_size = 1024 * 1024;
  if (getcontext (&ctx_new) != 0)
    FAIL_EXIT1 ("getcontext: %m");
  ctx_new.uc_stack.ss_sp = xmalloc (stack_size);
  ctx_new.uc_stack.ss_size = stack_size;
  ctx_new.uc_link = NULL;
  makecontext (&ctx_new, tcu_new_entry, 0);
  mode = MODE_SWAP;
  tcu_run (1);
}

static void
install_handler (int sig, int flags)
{
  static bool altstack_done;
  if (!altstack_done)
    {
      stack_t ss = { .ss_size = 256 * 1024 };
      ss.ss_sp = xmalloc (ss.ss_size);
      if (sigaltstack (&ss, NULL) != 0)
	FAIL_EXIT1 ("sigaltstack: %m");
      altstack_done = true;
    }
  struct sigaction sa = { .sa_sigaction = sample_handler,
			  .sa_flags = SA_SIGINFO | SA_ONSTACK | flags };
  sigemptyset (&sa.sa_mask);
  sigaddset (&sa.sa_mask, SIGPROF);
  sigaddset (&sa.sa_mask, SIGUSR1);
  xsigaction (sig, &sa, NULL);
}

static void
init_samples (int max)
{
  maxsamples = max;
  samples = xcalloc (max, sizeof (struct sample));
  nsamples = 0;
  /* The first unwind may allocate, not async-signal-safe.  */
  void *warm[2];
  unwind_backtrace (warm, 2);
}

/* Chain classification.  */

static void *libc_base, *vdso_base;

static void
init_libc_base (void)
{
  Dl_info info;
  void *sym = dlsym (RTLD_DEFAULT, "swapcontext");
  TEST_VERIFY_EXIT (sym != NULL);
  TEST_VERIFY_EXIT (dladdr (sym, &info) != 0);
  libc_base = info.dli_fbase;
  vdso_base = (void *) getauxval (AT_SYSINFO_EHDR);
}

/* libc or the vDSO (system call entry).  */
static bool
in_libc (uintptr_t addr)
{
  Dl_info info;
  return dladdr ((void *) addr, &info) != 0
	 && (info.dli_fbase == libc_base
	     || (vdso_base != NULL && info.dli_fbase == vdso_base));
}

/* Name of the test function containing ADDR, or NULL.  */
static const char *
func_name (uintptr_t addr)
{
  Dl_info info;
  const ElfW(Sym) *sym;
  if (dladdr1 ((void *) addr, &info, (void **) &sym, RTLD_DL_SYMENT) == 0
      || info.dli_sname == NULL || sym == NULL
      || strncmp (info.dli_sname, "tcu_", 4) != 0)
    return NULL;
  uintptr_t start = (uintptr_t) info.dli_saddr;
#if defined __powerpc64__ && _CALL_ELF != 2
  /* ELFv1 function symbols are function descriptors.  */
  start = *(uintptr_t *) start;
#elif defined __hppa__
  /* dladdr returns a function descriptor (plabel) for functions.  */
  if (start & 2)
    start = *(uintptr_t *) (start & ~3);
#endif
  if (addr < start || addr >= start + sym->st_size)
    return NULL;
  return info.dli_sname;
}

/* A chain lists the functions of a context from the innermost frame.
   ENDS_IN_LIBC chains belong to a makecontext context: after the last
   function at most one libc frame (the trampoline) may follow.  */
struct chain
{
  const char *name;
  const char *funcs[8];
  bool ends_in_libc;
};

static const struct chain chains[] =
{
  { "old/swap", { "tcu_old1", "tcu_old2", "tcu_old3", "tcu_run" }, false },
  { "old/set", { "tcu_switch", "tcu_old1", "tcu_old2", "tcu_old3",
		 "tcu_run" }, false },
  { "new/swap", { "tcu_new1", "tcu_new2", "tcu_new3", "tcu_new_entry" },
    true },
  { "new/set", { "tcu_switch", "tcu_new1", "tcu_new2", "tcu_new3",
		 "tcu_new_entry" }, true },
  { "tramp", { "tcu_run_tramp" }, false },
};

static int
chain_len (const struct chain *c)
{
  int n = 0;
  while (n < (int) (sizeof c->funcs / sizeof c->funcs[0]) && c->funcs[n])
    n++;
  return n;
}

enum verdict { V_BAD, V_TRUNC, V_CHAIN };

/* Frames after the interrupted one must be up to four libc frames
   followed by nothing (V_TRUNC) or by a complete chain (V_CHAIN).  */
static enum verdict
classify (const struct sample *s, const char **which)
{
  int i;
  for (i = 0; i < s->n; i++)
    if ((uintptr_t) s->frames[i] == s->pc)
      break;
  /* The ARM EHABI unwinder does not report a frame without unwind
     information.  */
  if (i == s->n)
    {
      *which = "no unwind information at pc";
      return V_TRUNC;
    }
  i++;

  /* Inside libc (the context functions) skip its frames.  Elsewhere
     outside the test functions (PLT, thunks) the chain may start at any
     function.  */
  const char *pcfunc = func_name (s->pc);
  bool pc_in_libc = in_libc (s->pc);
  if (pc_in_libc)
    {
      int lead = 0;
      while (i < s->n && in_libc ((uintptr_t) s->frames[i] - 1))
	i++, lead++;
      if (lead > 4)
	{
	  *which = "too many libc frames";
	  return V_BAD;
	}
      if (i == s->n)
	{
	  *which = "truncated";
	  return V_TRUNC;
	}
    }

  /* Outside the context switching code altogether.  */
  if (pcfunc == NULL)
    {
      int j = i;
      while (j < s->n && func_name ((uintptr_t) s->frames[j] - 1) == NULL)
	j++;
      if (j == s->n)
	{
	  *which = "outside";
	  return V_CHAIN;
	}
    }

  for (const struct chain *c = chains;
       c < chains + sizeof chains / sizeof chains[0]; c++)
    {
      int len = chain_len (c);
      int k = 0;
      const char *first = pcfunc;
      if (first == NULL && !pc_in_libc)
	first = func_name ((uintptr_t) s->frames[i] - 1);
      if (first != NULL)
	{
	  while (k < len && strcmp (c->funcs[k], first) != 0)
	    k++;
	  if (k == len)
	    continue;
	  if (pcfunc != NULL)
	    k++;
	}
      int j = i;
      for (; k < len && j < s->n; k++, j++)
	{
	  const char *f = func_name ((uintptr_t) s->frames[j] - 1);
	  if (f == NULL || strcmp (f, c->funcs[k]) != 0)
	    break;
	}
      if (k < len)
	continue;
      if (c->ends_in_libc
	  && (s->n - j > 1
	      || (s->n - j == 1
		  && !in_libc ((uintptr_t) s->frames[j] - 1))))
	continue;
      *which = c->name;
      return V_CHAIN;
    }
  *which = "no matching chain";
  return V_BAD;
}

static void
print_addr (const char *prefix, uintptr_t addr)
{
  Dl_info info;
  if (dladdr ((void *) addr, &info) != 0 && info.dli_sname != NULL)
    printf ("%s%#" PRIxPTR " %s+%#" PRIxPTR " (%s)\n", prefix, addr,
	    info.dli_sname, addr - (uintptr_t) info.dli_saddr,
	    info.dli_fname);
  else if (dladdr ((void *) addr, &info) != 0)
    printf ("%s%#" PRIxPTR " %s+%#" PRIxPTR "\n", prefix, addr,
	    info.dli_fname, addr - (uintptr_t) info.dli_fbase);
  else
    printf ("%s%#" PRIxPTR "\n", prefix, addr);
}

static void
print_sample (const struct sample *s)
{
  print_addr ("  pc: ", s->pc);
  for (int i = 0; i < s->n; i++)
    print_addr ("    ", (uintptr_t) s->frames[i]);
}

/* Check samples [FIRST, LAST).  Return the number of bad ones and store
   the number of truncated ones in *TRUNC.  */
static __attribute__ ((unused)) int
check_samples (const char *what, int first, int last, int *trunc,
	       bool verbose)
{
  int bad = 0, pc_in_libc = 0;
  *trunc = 0;
  for (int i = first; i < last; i++)
    {
      const char *which;
      enum verdict v = classify (&samples[i], &which);
      if (in_libc (samples[i].pc))
	pc_in_libc++;
#ifdef __arm__
      /* The EHABI unwinder looks up PC - 2 for the interrupted frame as
	 well, so the first instruction of a function is misattributed.  */
      Dl_info info;
      if (!in_libc (samples[i].pc)
	  && dladdr ((void *) samples[i].pc, &info) != 0
	  && (uintptr_t) info.dli_saddr == samples[i].pc)
	continue;
#endif
      if (v == V_TRUNC)
	++*trunc;
      if (v == V_BAD || verbose || (v == V_TRUNC && *trunc <= 3))
	{
	  printf ("%s: %s sample %d: %s\n", what,
		  v == V_BAD ? "error:" : "info:", i - first, which);
	  print_sample (&samples[i]);
	}
      if (v == V_BAD)
	bad++;
    }
  printf ("info: %s: %d samples, %d with pc in libc, %d truncated,"
	  " %d bad\n", what, last - first, pc_in_libc, *trunc, bad);
  return bad;
}
