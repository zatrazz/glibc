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

#ifndef	_EXIT_H
#define _EXIT_H 1

#include <stdbool.h>
#include <stdint.h>
#include <libc-lock.h>

enum
{
  ef_free,	/* `ef_free' MUST be zero!  */
  ef_us,
  ef_on,
  ef_at,
  ef_cxa
};

struct exit_function
  {
    /* `flavour' should be of type of the `enum' above but since we need
       this element in an atomic operation we have to use `long int'.  */
    long int flavor;
    union
      {
	void (*at) (void);
	struct
	  {
	    void (*fn) (int status, void *arg);
	    void *arg;
	  } on;
	struct
	  {
	    void (*fn) (void *arg, int status);
	    void *arg;
	    void *dso_handle;
	  } cxa;
      } func;
  };
/* A block is either one of the statically allocated RELRO initial blocks
   (see struct exit_functions_initial below) or, for overflow, a single
   mmap'd page.  Either way it is kept read-only at all times except for
   the brief window, always under __exit_funcs_lock, while an entry is
   being registered (see __new_exitfn) or marked ef_free (see
   __cxa_finalize and __run_exit_handlers).  MAX is the number of fns
   entries the block holds.  Keeping the pages read-only prevents an
   attacker with a write primitive from corrupting the stored
   (PTR_MANGLE'd) handler pointers between registration and exit.  */
struct exit_function_list
  {
    struct exit_function_list *next;
    size_t idx;
    size_t max;
    struct exit_function fns[];
  };

/* The atexit and quick_exit lists each keep a minimal block of statically
   allocated entries (EXIT_FUNCS_INITIAL_NFNS, the POSIX-required minimum)
   in the RELRO segment.  Registration of the first handlers therefore
   never depends on mmap, and the stored (PTR_MANGLE'd) pointers are
   read-only between registration and exit; mmap is only used for overflow
   beyond these blocks.

   Compared with reserving a whole page per list, this consumes far less
   memory in the common case: a program with a handful of handlers uses
   only these few kilobytes of RELRO and never calls mmap.  The object is
   placed at its natural alignment in the default RELRO segment, so it may
   straddle a page boundary; exit_funcs_setperm toggles every page the
   block occupies.  The trade-off is that the blocks are not page-exclusive,
   so while an entry is being registered or consumed during traversal the
   rest of the page(s) the object occupies (their other read-only data) is
   briefly writable.  Both blocks are kept together in one object (struct
   exit_functions_initial) so that, when it fits within a single page, only
   that one page is toggled.  */
#define EXIT_FUNCS_INITIAL_NFNS 32

/* Layout-compatible counterpart of struct exit_function_list with a fixed
   array, so a statically allocated instance can be used as an
   exit_function_list (see the _Static_assert in stdlib/cxa_atexit.c).  */
struct exit_function_initial
  {
    struct exit_function_list *next;
    size_t idx;
    size_t max;
    struct exit_function fns[EXIT_FUNCS_INITIAL_NFNS];
  };

/* Both lists' initial blocks, kept together (see above).  The members are
   not named 'atexit'/'quick_exit' because those identifiers are macros in
   the compat sources (e.g. old_atexit.c).  */
struct exit_functions_initial
  {
    struct exit_function_initial atexit_block;
    struct exit_function_initial quick_exit_block;
  };
extern struct exit_functions_initial __exit_funcs_initial attribute_hidden;

extern struct exit_function_list *__exit_funcs attribute_hidden;
extern struct exit_function_list *__quick_exit_funcs attribute_hidden;
extern uint64_t __new_exitfn_called attribute_hidden;

/* True once all registered atexit/at_quick_exit/onexit handlers have been
   called */
extern bool __exit_funcs_done attribute_hidden;

/* This lock protects __exit_funcs, __quick_exit_funcs, __exit_funcs_done
   and __new_exitfn_called globals against simultaneous access from
   atexit/on_exit/at_quick_exit in multiple threads, and also from
   simultaneous access while another thread is in the middle of calling
   exit handlers.  See BZ#14333.  Note: for lists, the entire list, and
   each associated entry in the list, is protected for all access by this
   lock.  */
__libc_lock_define (extern, __exit_funcs_lock);


extern struct exit_function *__new_exitfn (struct exit_function_list **listp)
  attribute_hidden;

/* Toggle the writability of the page-sized block that contains F.  Both
   must be called with __exit_funcs_lock held; they return 0 on success
   and -1 on failure.  __new_exitfn returns a slot whose block is left
   writable; the registrant fills the entry and then re-freezes it with
   __exit_funcs_protect.  */
extern int __exit_funcs_protect (struct exit_function *f) attribute_hidden;
extern int __exit_funcs_unprotect (struct exit_function *f) attribute_hidden;

extern void __run_exit_handlers (int status,
				 struct exit_function_list **listp,
				 bool run_list_atexit, bool run_dtors)
  attribute_hidden __attribute__ ((__noreturn__));

extern int __internal_atexit (void (*func) (void *), void *arg, void *d,
			      struct exit_function_list **listp)
  attribute_hidden;
extern int __cxa_at_quick_exit (void (*func) (void *), void *d);


#endif	/* exit.h  */
