/* Run initializers for newly loaded objects.
   Copyright (C) 1995-2026 Free Software Foundation, Inc.
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

#include <assert.h>
#include <atomic.h>
#include <stddef.h>
#include <ldsodefs.h>
#include <elf-initfini.h>
#include <dl-init-state.h>


void
_dl_init_schedule (struct link_map *main_map)
{
  /* Claim every map whose constructors the upcoming _dl_init call on
     MAIN_MAP is going to run.  Once GL(dl_load_lock) is released
     around the constructor calls, concurrent dlopen callers use this
     state to wait for the constructors to complete instead of
     observing the objects unconstructed.  */
  unsigned int i = main_map->l_searchlist.r_nlist;
  while (i-- > 0)
    {
      struct link_map *l = main_map->l_initfini[i];
      if (l == l->l_real && l->l_init_state == lm_init_not_called)
	{
	  l->l_init_thread = _dl_init_state_self ();
	  atomic_store_release (&l->l_init_state, lm_init_scheduled);
	}
    }
}

void
_dl_init_wait (struct link_map *l)
{
  l = l->l_real;
  for (;;)
    {
      unsigned int state = l->l_init_state;
      if ((state != lm_init_scheduled && state != lm_init_running)
	  || l->l_init_thread == _dl_init_state_self ())
	break;
      /* Release the lock so that the thread running the constructors
	 can complete them, and block until the state changes.
	 _dl_init_state_block returns immediately if the state word
	 has already moved on from STATE.  */
      __rtld_lock_unlock_recursive (GL(dl_load_lock));
      _dl_init_state_block (&l->l_init_state, state);
      __rtld_lock_lock_recursive (GL(dl_load_lock));
    }
}

/* Run the constructors of L if no other thread is doing so already.
   Called with GL(dl_load_lock) held at acquisition depth one; the
   lock is released while the constructors execute.  */
static void
call_init (struct link_map *l, int argc, char **argv, char **env)
{
  /* Do not run constructors for proxy objects.  */
  if (l != l->l_real)
    return;

  switch (l->l_init_state)
    {
    case lm_init_done:
      /* This object is all done.  */
      return;

    case lm_init_scheduled:
    case lm_init_running:
      if (l->l_init_thread != _dl_init_state_self ())
	{
	  /* Another thread's dlopen call is responsible for running
	     the constructors and may be executing them right now.
	     Wait for it, so that constructors of objects depending
	     on L only run after L has been constructed, and so that
	     the caller's dlopen does not return before L's
	     constructors finished.  */
	  _dl_init_wait (l);
	  return;
	}
      if (l->l_init_state == lm_init_running)
	/* Recursive call from L's own constructor, for example
	   through a circular dependency or a dlopen call with L in
	   the new object's dependency tree: there is nothing to
	   do.  */
	return;
      /* This dlopen call claimed L in _dl_init_schedule: run the
	 constructors below.  */
      break;

    case lm_init_not_called:
      /* No dlopen call has claimed L: this is the startup
	 initialization, or a map _dl_init_schedule did not cover.
	 Run the constructors here.  */
      break;
    }

  /* If the object has not been relocated, this is a bug.  The
     function pointers are invalid in this case.  (Executables do not
     need relocation.)  */
  assert (l->l_relocated || l->l_type == lt_executable);

  /* Avoid handling this constructor again in case we have a circular
     dependency.  The lm_init_running state also directs concurrent
     dlopen callers of L to wait for the constructors to finish.  */
  l->l_init_thread = _dl_init_state_self ();
  atomic_store_release (&l->l_init_state, lm_init_running);

  /* Check for object which constructors we do not run here: the
     startup code takes care of running the constructors of the main
     executable, but the map still needs to reach the final state.  */
  if (!(__builtin_expect (l->l_name[0], 'a') == '\0'
	&& l->l_type == lt_executable))
    {
      /* Print a debug message if wanted.  */
      if (__glibc_unlikely (GLRO(dl_debug_mask) & DL_DEBUG_IMPCALLS))
	_dl_debug_printf ("\ncalling init: %s\n\n",
			  DSO_FILENAME (l->l_name));

#if DL_INIT_STATE_RELEASE_LOCK
      /* Run the constructors without GL(dl_load_lock): they may use
	 arbitrary libc functionality, including code that takes the
	 lock from this or another thread, without deadlocking
	 (BZ 15686).  The caller guarantees an acquisition depth of
	 exactly one, so the unlock below releases the lock.  */
      __rtld_lock_unlock_recursive (GL(dl_load_lock));
#endif

      /* Now run the local constructors.  There are two forms of them:
	 - the one named by DT_INIT
	 - the others in the DT_INIT_ARRAY.
      */
      if (ELF_INITFINI && l->l_info[DT_INIT] != NULL)
	DL_CALL_DT_INIT(l, l->l_addr + l->l_info[DT_INIT]->d_un.d_ptr,
			argc, argv, env);

      /* Next see whether there is an array with initialization
	 functions.  */
      ElfW(Dyn) *init_array = l->l_info[DT_INIT_ARRAY];
      if (init_array != NULL)
	{
	  unsigned int j;
	  unsigned int jm;
	  ElfW(Addr) *addrs;

	  jm = l->l_info[DT_INIT_ARRAYSZ]->d_un.d_val / sizeof (ElfW(Addr));

	  addrs = (ElfW(Addr) *) (init_array->d_un.d_ptr + l->l_addr);
	  for (j = 0; j < jm; ++j)
	    ((dl_init_t) addrs[j]) (argc, argv, env);
	}

#if DL_INIT_STATE_RELEASE_LOCK
      __rtld_lock_lock_recursive (GL(dl_load_lock));
#endif
    }

  atomic_store_release (&l->l_init_state, lm_init_done);
  _dl_init_state_wake (&l->l_init_state);
}


void
_dl_init (struct link_map *main_map, int argc, char **argv, char **env)
{
  ElfW(Dyn) *preinit_array = main_map->l_info[DT_PREINIT_ARRAY];
  ElfW(Dyn) *preinit_array_size = main_map->l_info[DT_PREINIT_ARRAYSZ];
  unsigned int i;

  /* The constructor state changes require GL(dl_load_lock), which no
     caller of _dl_init holds (call_init needs the acquisition depth
     to be exactly one so that it can release the lock around the
     constructor invocations).  */
  __rtld_lock_lock_recursive (GL(dl_load_lock));

  if (__glibc_unlikely (GL(dl_initfirst) != NULL))
    {
      call_init (GL(dl_initfirst), argc, argv, env);
      GL(dl_initfirst) = NULL;
    }

  __rtld_lock_unlock_recursive (GL(dl_load_lock));

  /* Don't do anything if there is no preinit array.  */
  if (__builtin_expect (preinit_array != NULL, 0)
      && preinit_array_size != NULL
      && (i = preinit_array_size->d_un.d_val / sizeof (ElfW(Addr))) > 0)
    {
      ElfW(Addr) *addrs;
      unsigned int cnt;

      if (__glibc_unlikely (GLRO(dl_debug_mask) & DL_DEBUG_IMPCALLS))
	_dl_debug_printf ("\ncalling preinit: %s\n\n",
			  DSO_FILENAME (main_map->l_name));

      addrs = (ElfW(Addr) *) (preinit_array->d_un.d_ptr + main_map->l_addr);
      for (cnt = 0; cnt < i; ++cnt)
	((dl_init_t) addrs[cnt]) (argc, argv, env);
    }

  /* Stupid users forced the ELF specification to be changed.  It now
     says that the dynamic loader is responsible for determining the
     order in which the constructors have to run.  The constructors
     for all dependencies of an object must run before the constructor
     for the object itself.  Circular dependencies are left unspecified.

     This is highly questionable since it puts the burden on the dynamic
     loader which has to find the dependencies at runtime instead of
     letting the user do it right.  Stupidity rules!  */

  __rtld_lock_lock_recursive (GL(dl_load_lock));

  i = main_map->l_searchlist.r_nlist;
  while (i-- > 0)
    call_init (main_map->l_initfini[i], argc, argv, env);

  __rtld_lock_unlock_recursive (GL(dl_load_lock));

#ifndef HAVE_INLINED_SYSCALLS
  /* Finished starting up.  */
  _dl_starting_up = 0;
#endif
}
