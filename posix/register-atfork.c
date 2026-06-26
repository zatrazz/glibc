/* Copyright (C) 2002-2026 Free Software Foundation, Inc.
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

#include <libc-lock.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <register-atfork.h>
#include <intprops.h>
#include <list.h>
#include <stdio.h>

/* New handlers are added at the front, so the list runs from the newest
   (highest ID) at the front to the oldest (lowest ID) at the back.  A new
   node is allocated with malloc only when no reusable node is available, and
   always *outside* of ATFORK_LOCK: holding ATFORK_LOCK across an allocation
   would deadlock with interposable allocators whose own locks are also taken
   from an atfork handler.  */
static LIST_HEAD (fork_handlers);

/* Unregistered nodes are moved here (under ATFORK_LOCK) instead of being
   freed, and a subsequent registration reuses one.  This keeps both
   registration and unregistration allocation-free in the steady state (in
   particular avoids any free under ATFORK_LOCK).  The pool is released only
   by __libc_atfork_freemem.  */
static LIST_HEAD (fork_handlers_free);

static uint64_t fork_handler_counter;

/* Bumped on every list mutation (registration or unregistration).  The fork
   handler runners drop ATFORK_LOCK while a handler executes; comparing this
   counter across that window tells them whether the list changed and a saved
   list position is therefore still valid.  */
static uint64_t fork_handler_modcount;

static int atfork_lock = LLL_LOCK_INITIALIZER;

#define fork_handler_entry(pos) list_entry (pos, struct fork_handler, list)

/* True if the list LP has no elements.  */
static inline bool
list_is_empty (list_t *lp)
{
  return lp->next == lp;
}

/* Initialize NEWP from the supplied handlers and link it at the front of the
   active list.  ATFORK_LOCK must be held.  */
static void
fork_handler_init (struct fork_handler *newp, void (*prepare) (void),
		   void (*parent) (void), void (*child) (void),
		   void *dso_handle)
{
  newp->prepare_handler = prepare;
  newp->parent_handler = parent;
  newp->child_handler = child;
  newp->dso_handle = dso_handle;

  /* IDs assigned to handlers start at 1 and increment with handler
     registration.  Un-registering a handler discards the corresponding ID.
     It is not reused in future registrations.  */
  if (INT_ADD_OVERFLOW (fork_handler_counter, 1))
    __libc_fatal ("fork handler counter overflow");
  newp->id = ++fork_handler_counter;

  /* Add at the front: the highest ID ends up first.  */
  list_add (&newp->list, &fork_handlers);

  ++fork_handler_modcount;
}

int
__register_atfork (void (*prepare) (void), void (*parent) (void),
		   void (*child) (void), void *dso_handle)
{
  lll_lock (atfork_lock, LLL_PRIVATE);

  bool free_empty = list_is_empty (&fork_handlers_free);
  if (!free_empty)
    {
      list_t *reuse = fork_handlers_free.next;
      list_del (reuse);
      fork_handler_init (fork_handler_entry (reuse), prepare, parent, child,
			 dso_handle);
    }

  lll_unlock (atfork_lock, LLL_PRIVATE);

  if (!free_empty)
    return 0;

  struct fork_handler *newp = malloc (sizeof (*newp));
  if (newp == NULL)
    return ENOMEM;

  lll_lock (atfork_lock, LLL_PRIVATE);
  fork_handler_init (newp, prepare, parent, child, dso_handle);
  lll_unlock (atfork_lock, LLL_PRIVATE);

  return 0;
}
libc_hidden_def (__register_atfork)

void
__unregister_atfork (void *dso_handle)
{
  list_t *runp, *prevp;

  lll_lock (atfork_lock, LLL_PRIVATE);

  /* Move the matching nodes to the free pool rather than freeing them: this
     avoids a free under ATFORK_LOCK and lets a later registration reuse
     them.  */
  list_for_each_prev_safe (runp, prevp, &fork_handlers)
    if (fork_handler_entry (runp)->dso_handle == dso_handle)
      {
	list_del (runp);
	list_add (runp, &fork_handlers_free);
	++fork_handler_modcount;
      }

  lll_unlock (atfork_lock, LLL_PRIVATE);
}

uint64_t
__run_prefork_handlers (_Bool do_locking)
{
  uint64_t lastrun;

  if (do_locking)
    lll_lock (atfork_lock, LLL_PRIVATE);

  /* We run prepare handlers from newest to oldest (highest ID first).  After
     fork, only handlers up to the last handler found here (pre-fork) will be
     run.  Handlers registered during __run_prefork_handlers or
     __run_postfork_handlers will have a higher ID, and since their prepare
     handlers will not be run now, their parent/child handlers should also be
     ignored.  */
  lastrun = fork_handler_counter;

  /* The newest handlers are at the front; skip any with ID > LASTRUN (those
     were registered after this function was entered).  */
  list_t *pos = fork_handlers.next;
  while (pos != &fork_handlers && fork_handler_entry (pos)->id > lastrun)
    pos = pos->next;

  while (pos != &fork_handlers)
    {
      struct fork_handler *runp = fork_handler_entry (pos);
      uint64_t id = runp->id;
      void (*prepare_handler) (void) = runp->prepare_handler;

      /* Remember where to continue and whether the list changed across the
         handler call.  Both are read while the lock is held.  */
      uint64_t saved_modcount = fork_handler_modcount;
      list_t *next = pos->next;

      if (prepare_handler != NULL)
        {
          if (do_locking)
            lll_unlock (atfork_lock, LLL_PRIVATE);

          prepare_handler ();

          if (do_locking)
            lll_lock (atfork_lock, LLL_PRIVATE);
        }

      /* Advance to the next (lower ID) handler.  If nothing was registered or
         unregistered while the lock was dropped, the saved position is still
         valid.  Otherwise it may be stale (a node could even have been
         freed), so re-find the next handler by ID: the one with the greatest
         ID strictly below the just-run handler.  */
      if (fork_handler_modcount == saved_modcount)
        pos = next;
      else
        {
          pos = fork_handlers.next;
          while (pos != &fork_handlers && fork_handler_entry (pos)->id >= id)
            pos = pos->next;
        }
    }

  return lastrun;
}

void
__run_postfork_handlers (enum __run_fork_handler_type who, _Bool do_locking,
                         uint64_t lastrun)
{
  /* Run parent/child handlers from oldest to newest (lowest ID first).  The
     oldest handlers are at the back of the list.  */
  list_t *pos = fork_handlers.prev;
  while (pos != &fork_handlers)
    {
      struct fork_handler *runp = fork_handler_entry (pos);
      uint64_t id = runp->id;

      /* prepare handlers were not run for handlers with ID > LASTRUN.
         Thus, parent/child handlers will also not be run.  */
      if (id > lastrun)
        break;

      void (*handler) (void) = NULL;
      if (who == atfork_run_child)
        handler = runp->child_handler;
      else if (who == atfork_run_parent)
        handler = runp->parent_handler;

      uint64_t saved_modcount = fork_handler_modcount;
      list_t *prev = pos->prev;

      if (handler != NULL)
        {
          if (do_locking)
            lll_unlock (atfork_lock, LLL_PRIVATE);

          handler ();

          if (do_locking)
            lll_lock (atfork_lock, LLL_PRIVATE);
        }

      /* Advance to the next (higher ID) handler.  Fast path when the list did
         not change; otherwise re-find by ID the handler with the smallest ID
         strictly above the just-run one.  */
      if (fork_handler_modcount == saved_modcount)
        pos = prev;
      else
        {
          pos = fork_handlers.prev;
          while (pos != &fork_handlers && fork_handler_entry (pos)->id <= id)
            pos = pos->prev;
        }
    }

  if (do_locking)
    lll_unlock (atfork_lock, LLL_PRIVATE);
}


void
__libc_atfork_freemem (void)
{
  /* Detach both the active list and the free pool under the lock and free
     them afterwards.  */
  LIST_HEAD (removed);
  list_t *runp, *prevp;

  lll_lock (atfork_lock, LLL_PRIVATE);

  list_splice (&fork_handlers, &removed);
  INIT_LIST_HEAD (&fork_handlers);

  list_splice (&fork_handlers_free, &removed);
  INIT_LIST_HEAD (&fork_handlers_free);

  lll_unlock (atfork_lock, LLL_PRIVATE);

  list_for_each_prev_safe (runp, prevp, &removed)
    free (fork_handler_entry (runp));
}
