/* Implementation of the getrandom system call.
   Copyright (C) 2016-2024 Free Software Foundation, Inc.
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

#include <sys/random.h>
#include <errno.h>
#include <unistd.h>
#include <sysdep-cancel.h>

#ifdef HAVE_GETRANDOM_VSYSCALL
# include <getrandom_vdso.h>
# include <ldsodefs.h>
# include <libc-lock.h>
# include <list.h>
# include <setvmaname.h>
# include <sys/mman.h>
# include <sys/sysinfo.h>
# include <tls-internal.h>

# define ALIGN_PAGE(p)		PTR_ALIGN_UP (p, GLRO (dl_pagesize))
# define READ_ONCE(p)		(*((volatile typeof (p) *) (&(p))))
# define WRITE_ONCE(p, v)	(*((volatile typeof (p) *) (&(p))) = (v))
# define RESERVE_PTR(p)		((void *) ((uintptr_t) (p) | 1UL))
# define RELEASE_PTR(p)		((void *) ((uintptr_t) (p) & ~1UL))
# define IS_RESERVED_PTR(p)	(!!((uintptr_t) (p) & 1UL))

static struct
{
  /* Must be held always on access, as this is used by multiple threads.  */
  __libc_lock_define (, lock);

  /* Stack of opaque states for use in vgetrandom.  */
  void **states;

  /* Size of each opaque state, copied from vgetrandom_opaque_params.  */
  size_t state_size;

  /* Number of states available in the queue.  */
  size_t len;

  /* Number of states in the queue plus the number of states used in
     threads.  */
  size_t total;

  /* Number of states that the states array can hold before needing to be
     resized.  */
  size_t cap;
} grnd_alloc = { .lock = LLL_LOCK_INITIALIZER };

/* Allocate an opaque state for vgetrandom.  If the allocator does not have
   any, mmap() another page of them.  */
static void *
vgetrandom_get_state (void)
{
  void *state = NULL;

  /* The signal blocking avoid the potential issue where _Fork() (which is
     async-signal-safe) is called with the lock taken.  The function is called
     only once during thread lifetime, so the overhead should be ok.  */
  internal_sigset_t set;
  internal_signal_block_all (&set);
  __libc_lock_lock (grnd_alloc.lock);

  if (grnd_alloc.len == 0)
    {
      struct vgetrandom_opaque_params params;
      size_t block_size, num = __get_nprocs (); /* Just a decent heuristic. */
      void *block;

      if (GLRO (dl_vdso_getrandom) (NULL, 0, 0, &params, ~0UL) != 0)
        goto out;
      grnd_alloc.state_size = params.size_of_opaque_state;

      block_size = ALIGN_PAGE (num * grnd_alloc.state_size);
      num = (GLRO (dl_pagesize) / grnd_alloc.state_size) *
            (block_size / GLRO (dl_pagesize));
      block = __mmap (0, block_size, params.mmap_prot,
                      params.mmap_flags, -1, 0);
      if (block == MAP_FAILED)
        goto out;
      __set_vma_name (block, block_size, " glibc: getrandom");

      if (grnd_alloc.total + num > grnd_alloc.cap)
        {
          void **states;
	  size_t states_size;
	  /* Use a new mmap instead of trying to mremap.  It avoids a
	     potential multithread fork issue where fork is called just after
	     mremap returns but before assigning to the grnd_alloc.states,
	     thus making the its value invalid in the child.  */
	  void *old_states = grnd_alloc.states;
	  size_t old_states_size = ALIGN_PAGE (sizeof (*grnd_alloc.states) *
					       grnd_alloc.total + num);
	  if (grnd_alloc.states == NULL)
	    states_size = old_states_size;
	  else
	    states_size = ALIGN_PAGE (sizeof (*grnd_alloc.states)
				      * grnd_alloc.cap);

	  states = __mmap (NULL, states_size, PROT_READ | PROT_WRITE,
			   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
          if (states == MAP_FAILED)
            goto unmap;

	  /* Atomically replace the old state, so if a fork happens the child
	     process will see a consistent free state buffer.  The size might
	     not be updated, but it does not really matter since the buffer is
	     always increased.  */
          atomic_store_relaxed (&grnd_alloc.states, states);
	  if (old_states != NULL)
	    __munmap (old_states, old_states_size);

          __set_vma_name (states, states_size, " glibc: getrandom states");
          grnd_alloc.cap = states_size / sizeof (*grnd_alloc.states);
        }

      for (size_t i = 0; i < num; ++i)
        {
          /* States should not straddle a page.  */
          if (((uintptr_t) block & (GLRO (dl_pagesize) - 1)) +
              grnd_alloc.state_size > GLRO (dl_pagesize))
            block = ALIGN_PAGE (block);
          grnd_alloc.states[i] = block;
          block += grnd_alloc.state_size;
        }
      grnd_alloc.len = num;
      grnd_alloc.total += num;
      goto success;

    unmap:
      __munmap (block, block_size);
      goto out;
    }

success:
  state = grnd_alloc.states[--grnd_alloc.len];

out:
  __libc_lock_unlock (grnd_alloc.lock);
  internal_signal_restore_set (&set);
  return state;
}

/* Returns true when vgetrandom is used successfully.  Returns false if the
   syscall fallback should be issued in the case the vDSO is not present, in
   the case of reentrancy, or if any memory allocation fails.  */
static bool
__getrandom_vdso (ssize_t *ret, void *buffer, size_t length,
                  unsigned int flags)
{
  if (GLRO (dl_vdso_getrandom) == NULL)
    return false;

  struct pthread *self = THREAD_SELF;

  /* If the LSB of getrandom_buf is set, then this function is already being
     called, and we have a reentrant call from a signal handler.  In this case
     fallback to the syscall.  */
  void *state = READ_ONCE (self->getrandom_buf);
  if (IS_RESERVED_PTR (state))
    return false;
  WRITE_ONCE (self->getrandom_buf, RESERVE_PTR (state));

  bool r = false;
  if (state == NULL)
    {
      state = vgetrandom_get_state ();
      if (state == NULL)
        goto out;
    }

  *ret = GLRO (dl_vdso_getrandom) (buffer, length, flags, state,
                                   grnd_alloc.state_size);
  if (INTERNAL_SYSCALL_ERROR_P (*ret))
    {
      __set_errno (INTERNAL_SYSCALL_ERRNO (*ret));
      *ret = -1;
    }
  r = true;

out:
  WRITE_ONCE (self->getrandom_buf, state);
  return r;
}
#endif

/* Re-add the state state from CURP on the free list.  */
void
__getrandom_reset_state (struct pthread *curp)
{
#ifdef HAVE_GETRANDOM_VSYSCALL
  if (curp->getrandom_buf == NULL)
    return;
  grnd_alloc.states[grnd_alloc.len++] = RELEASE_PTR (curp->getrandom_buf);
  curp->getrandom_buf = NULL;
#endif
}

/* Called when a thread terminates, and adds its random buffer back into the
   allocator pool for use in a future thread.  */
void
__getrandom_vdso_release (struct pthread *curp)
{
#ifdef HAVE_GETRANDOM_VSYSCALL
  if (curp->getrandom_buf == NULL)
    return;

  __libc_lock_lock (grnd_alloc.lock);
  grnd_alloc.states[grnd_alloc.len++] = curp->getrandom_buf;
  __libc_lock_unlock (grnd_alloc.lock);
#endif
}

/* Reset the internal lock state in case another thread has locked while
   this thread calls fork.  The stale thread states will be handled by
   reclaim_stacks which calls __getrandom_reset_state on each thread.  */
void
__getrandom_fork_subprocess (void)
{
#ifdef HAVE_GETRANDOM_VSYSCALL
  grnd_alloc.lock = LLL_LOCK_INITIALIZER;
#endif
}

ssize_t
__getrandom_nocancel (void *buffer, size_t length, unsigned int flags)
{
#ifdef HAVE_GETRANDOM_VSYSCALL
  ssize_t r;
  if (__getrandom_vdso (&r, buffer, length, flags))
    return r;
#endif

  return INLINE_SYSCALL_CALL (getrandom, buffer, length, flags);
}

/* Write up to LENGTH bytes of randomness starting at BUFFER.
   Return the number of bytes written, or -1 on error.  */
ssize_t
__getrandom (void *buffer, size_t length, unsigned int flags)
{
#ifdef HAVE_GETRANDOM_VSYSCALL
  ssize_t r;
  if (__getrandom_vdso (&r, buffer, length, flags))
    return r;
#endif

  return INTERNAL_SYSCALL_CALL (getrandom, buffer, length, flags);
}
libc_hidden_def (__getrandom)
weak_alias (__getrandom, getrandom)
