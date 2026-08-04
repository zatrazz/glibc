/* Waiting for ELF constructor state changes.  NPTL version.
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

#ifndef _DL_INIT_STATE_H
#define _DL_INIT_STATE_H

#include <limits.h>
#include <lowlevellock-futex.h>
#include <single-thread.h>
#include <tls.h>

/* Threads can block on l_init_state changes with a futex, so the
   dynamic loader releases GL(dl_load_lock) around ELF constructor
   execution (BZ 15686).  */
#define DL_INIT_STATE_RELEASE_LOCK 1

/* Identity of the running thread, used to detect recursive
   constructor execution.  */
static inline void *
_dl_init_state_self (void)
{
  return THREAD_SELF;
}

/* Block until *STATE changes from STATE_VAL.  Spurious returns are
   fine; the callers reevaluate the state under GL(dl_load_lock).  */
static inline void
_dl_init_state_block (unsigned int *state, unsigned int state_val)
{
  lll_futex_wait ((int *) state, state_val, LLL_PRIVATE);
}

/* Wake all threads blocked in _dl_init_state_block on STATE.  */
static inline void
_dl_init_state_wake (unsigned int *state)
{
  /* Waiters imply that more than one thread exists.  */
  if (!RTLD_SINGLE_THREAD_P)
    lll_futex_wake ((int *) state, INT_MAX, LLL_PRIVATE);
}

#endif /* _DL_INIT_STATE_H */
