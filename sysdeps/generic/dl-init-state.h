/* Waiting for ELF constructor state changes.  Generic version.
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

/* The generic version provides no way for a thread to block until
   another thread finishes running ELF constructors, so the dynamic
   loader keeps running constructors with GL(dl_load_lock) held.
   Because the lock is then never released while a map is in the
   lm_init_scheduled or lm_init_running state, no thread can observe
   another thread's constructor execution, and the identity returned
   by _dl_init_state_self is never compared against one recorded by
   a different thread.  */
#define DL_INIT_STATE_RELEASE_LOCK 0

/* Identity of the running thread, used to detect recursive
   constructor execution.  */
static inline void *
_dl_init_state_self (void)
{
  return NULL;
}

/* Block until *STATE changes from STATE_VAL.  Unused when
   DL_INIT_STATE_RELEASE_LOCK is 0.  */
static inline void
_dl_init_state_block (unsigned int *state, unsigned int state_val)
{
}

/* Wake all threads blocked in _dl_init_state_block on STATE.  Unused
   when DL_INIT_STATE_RELEASE_LOCK is 0.  */
static inline void
_dl_init_state_wake (unsigned int *state)
{
}

#endif /* _DL_INIT_STATE_H */
