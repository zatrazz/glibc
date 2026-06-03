/* Copyright (C) 1991-2026 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pointer_guard.h>
#include <libc-lock.h>
#include <ldsodefs.h>
#include <set-freeres.h>
#include "exit.h"

/* Initialize the flag that indicates exit function processing
   is complete. See concurrency notes in stdlib/exit.h where
   __exit_funcs_lock is declared.  */
bool __exit_funcs_done = false;

/* The lock handles concurrent exit() and quick_exit(), even though the
   C/POSIX standard states that calling exit() more than once is UB.  The
   recursive lock allows atexit() handlers or destructors to call exit()
   itself.  In this case, the  handler list execution will resume at the
   point of the current handler.  */
__libc_lock_define_initialized_recursive (static, __exit_lock)

/* Call all functions registered with `atexit' and `on_exit',
   in the reverse of the order in which they were registered
   perform stdio cleanup, and terminate program execution with STATUS.  */
void
attribute_hidden
__run_exit_handlers (int status, struct exit_function_list **listp,
		     bool run_list_atexit, bool run_dtors)
{
  /* The exit should never return, so there is no need to unlock it.  */
  __libc_lock_lock_recursive (__exit_lock);

  /* First, call the TLS destructors.  */
  if (run_dtors)
    call_function_static_weak (__call_tls_dtors);

  __libc_lock_lock (__exit_funcs_lock);

  /* We do it this way to handle recursive calls to exit () made by
     the functions registered with `atexit' and `on_exit'. We call
     everyone on the list and use the status value in the last
     exit (). */
  while (true)
    {
      struct exit_function_list *cur;

    restart:
      cur = *listp;

      if (cur == NULL)
	{
	  /* Exit processing complete.  We will not allow any more
	     atexit/on_exit registrations.  */
	  __exit_funcs_done = true;
	  break;
	}

      while (cur->idx > 0)
	{
	  const uint64_t new_exitfn_called = __new_exitfn_called;

	  /* Consuming an entry requires writing into the frozen block: the
	     idx decrement below and, for ef_cxa, the ef_free mark.  Do both
	     under a brief unprotect, take a local copy of the entry, then
	     re-freeze the block before running the (foreign) handler so the
	     remaining mangled pointers stay read-only across the call.  If
	     the block cannot be unfrozen we cannot safely make progress.  */
	  if (__exit_funcs_unprotect (&cur->fns[0]) != 0)
	    break;
	  struct exit_function *const f = &cur->fns[--cur->idx];
	  const struct exit_function ef = *f;
	  if (f->flavor == ef_cxa)
	    /* To avoid dlclose/exit race calling cxafct twice (BZ 22180),
	       we must mark this function as ef_free.  */
	    f->flavor = ef_free;
	  __exit_funcs_protect (&cur->fns[0]);

	  switch (ef.flavor)
	    {
	      void (*atfct) (void);
	      void (*onfct) (int status, void *arg);
	      void (*cxafct) (void *arg, int status);

	    case ef_free:
	    case ef_us:
	      break;
	    case ef_on:
	      onfct = ef.func.on.fn;
	      PTR_DEMANGLE (onfct);

	      /* Unlock the list while we call a foreign function.  */
	      __libc_lock_unlock (__exit_funcs_lock);
	      onfct (status, ef.func.on.arg);
	      __libc_lock_lock (__exit_funcs_lock);
	      break;
	    case ef_at:
	      atfct = ef.func.at;
	      PTR_DEMANGLE (atfct);

	      /* Unlock the list while we call a foreign function.  */
	      __libc_lock_unlock (__exit_funcs_lock);
	      atfct ();
	      __libc_lock_lock (__exit_funcs_lock);
	      break;
	    case ef_cxa:
	      cxafct = ef.func.cxa.fn;
	      PTR_DEMANGLE (cxafct);

	      /* Unlock the list while we call a foreign function.  */
	      __libc_lock_unlock (__exit_funcs_lock);
	      cxafct (ef.func.cxa.arg, status);
	      __libc_lock_lock (__exit_funcs_lock);
	      break;
	    }

	  if (__glibc_unlikely (new_exitfn_called != __new_exitfn_called))
	    /* The last exit function, or another thread, has registered
	       more exit functions.  Start the loop over.  */
	    goto restart;
	}

      *listp = cur->next;
      if (*listp != NULL)
	/* CUR is an mmap'd overflow block; return it to the system.  The
	   static fallback block is the permanent tail (next == NULL) and
	   is never unmapped.  A recursive exit() triggered by a handler
	   resumes by reloading *listp at the restart label above, and CUR
	   is only unmapped once fully drained and unlinked, so it is never
	   revisited.  */
	__munmap (cur, GLRO (dl_pagesize));
    }

  __libc_lock_unlock (__exit_funcs_lock);

  if (run_list_atexit)
    call_function_static_weak (_IO_cleanup);

  _exit (status);
}


void
exit (int status)
{
  __run_exit_handlers (status, &__exit_funcs, true, true);
}
libc_hidden_def (exit)
