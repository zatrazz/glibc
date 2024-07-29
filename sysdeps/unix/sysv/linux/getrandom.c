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

#include <libc-lock.h>
#include <sys/random.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <unistd.h>
#include <sysdep-cancel.h>
#include <ldsodefs.h>
#include <tls-internal.h>

#ifdef HAVE_GETRANDOM_VSYSCALL
# include <getrandom_vdso.h>
# include <setvmaname.h>
# include <list.h>
# include <internal-signals.h>

/* The vDSO symbol operates in an opaque state, where it should be
   allocated with mmap using the kernel-provided protection and flags.
   It is done by issuing the getrandom vDSO symbol with the additional
   vgetrandom_opaque_params argument.  Multiple states are allocated, so
   it provides per-thread support without locking.

   To keep track and re-use the opaque state when threads exit, getrandom
   keeps track of the available ones in FIFO.  When there is no available
   opaque state, a new one is allocated.

   Also, to make the vDSO call async-signal-safe, two strategies are used:

      1. The per-thread opaque state is 'taken' with signal_exchange_value,
         and 'released' after its use  Reentrancy falls back to the syscall.

      2. The opaque state control block is allocated with mmap.

   The opaque state control block also needs to keep track of allocated
   blocks, so it can be put again on the available free states.  Currently,
   only one page is assigned to the control block.  */

static struct
{
  __libc_lock_define (, lock);
  void **states;
  size_t len;		/* The number of available free states.  */
  size_t cap;		/* Total slots from vgetrandom_alloc_syscall.  */
  size_t maxcap;	/* Maximum allocated states.  */
  size_t state_size;	/* Size of each state.  */
} grnd_allocator =
{
  .lock = LLL_LOCK_INITIALIZER
};

static bool
vgetrandom_alloc_state_array (void *new_block, size_t new_cap)
{
  if (grnd_allocator.maxcap == 0)
    {
      size_t new_size;
      size_t state_size = sizeof (*grnd_allocator.states);
      if (__builtin_mul_overflow (new_cap, state_size, &new_size))
	return false;

      size_t alloc_size = ALIGN_UP (new_size, GLRO (dl_pagesize));
      void **new_states = __mmap (NULL, alloc_size, PROT_READ | PROT_WRITE,
				  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (new_states == MAP_FAILED)
	return false;
      __set_vma_name (new_states, alloc_size, " glibc: getrandom");

      grnd_allocator.maxcap = alloc_size / state_size ;
      grnd_allocator.states = new_states;
      return true;
    }
  return new_cap < grnd_allocator.maxcap;
}

static void
vgetrandom_init_states (void *new_block)
{
  for (size_t i = 0; i < grnd_allocator.len; i++)
    {
      if (((uintptr_t)new_block & (GLRO(dl_pagesize) - 1))
	  + grnd_allocator.state_size > GLRO(dl_pagesize))
	new_block = PTR_ALIGN_UP (new_block, GLRO(dl_pagesize));
      grnd_allocator.states[i] = new_block;
      new_block += grnd_allocator.state_size;
    }
}

static struct grnd_allocator_state *
vgetrandom_get_state (void)
{
  void *state = NULL;

  /* Avoid the small window where getrandom() is interrupted by signal
     handler, and it calls _Fork().  The lock is reset by the _Fork(),
     but when it returns from signal handler, the unlock will put it in
     an inconsistent state.  */
  internal_sigset_t set;
  internal_signal_block_all (&set);
  __libc_lock_lock (grnd_allocator.lock);

  if (grnd_allocator.len == 0)
    {
      size_t num_hint = __get_nprocs ();

      struct vgetrandom_opaque_params params;
      if (GLRO(dl_vdso_getrandom)(NULL, 0, 0, &params, ~0UL) != 0)
	goto out;
      grnd_allocator.state_size = params.size_of_opaque_state;

      size_t alloc_size = ALIGN_UP (num_hint * grnd_allocator.state_size,
				    GLRO(dl_pagesize));
      void *new_block = __mmap (NULL, alloc_size, params.mmap_prot,
				params.mmap_flags, -1, 0);
      if (new_block == MAP_FAILED)
	goto out;

      size_t num = (GLRO(dl_pagesize) / grnd_allocator.state_size)
		   * (alloc_size / GLRO(dl_pagesize));
      size_t new_cap;
      if (__builtin_add_overflow (grnd_allocator.cap, num, &new_cap)
	  || !vgetrandom_alloc_state_array (new_block, new_cap))
	goto unmap;
      grnd_allocator.cap = new_cap;
      grnd_allocator.len = num;

      vgetrandom_init_states (new_block);
      goto success;

    unmap:
      __munmap (new_block, alloc_size);
      goto out;
    }

success:
  state = grnd_allocator.states[--grnd_allocator.len];

out:
  __libc_lock_unlock (grnd_allocator.lock);
  internal_signal_restore_set (&set);
  return state;
}

#define READ_ONCE(x) \
  ({ (void*) (*(const volatile void **)&(x)); })
#define WRITE_ONCE(x, val) \
  ({ *(volatile void **)&(x) = (val); })

#define MASK_VALUE(val) \
  ({ (void *)(((uintptr_t)val) | 1UL); })

#define UNMASK_VALUE(val) \
  ({ (void *)(((uintptr_t)val) & ~1UL); })

/* Return true if the syscall fallback should be issued in the case the vDSO
   is not present, in the case of reentrancy, or if any memory allocation
   fails.  */
static bool
__getrandom_internal (ssize_t *ret, void *buffer, size_t length,
                      unsigned int flags)
{
  if (GLRO(dl_vdso_getrandom) == NULL)
    return false;

  /* Handles reentracy from a signal handler.  It assumes that a pointer read
     and write is atomic w.r.t to signal handler (similar to sig_atomic_t).  */
  struct tls_internal_t *ti = __glibc_tls_internal ();
   void *state = READ_ONCE (ti->getrandom_buf);
  if ((uintptr_t)state & 1UL)
    return false;
  WRITE_ONCE (ti->getrandom_buf, MASK_VALUE (state));

  bool r = false;
  if (state == NULL)
    {
      state = vgetrandom_get_state ();
      if (state == NULL)
        goto out;
    }

  *ret = GLRO(dl_vdso_getrandom)(buffer, length, flags, state,
				 grnd_allocator.state_size);
  if (INTERNAL_SYSCALL_ERROR_P (*ret))
    {
      __set_errno (INTERNAL_SYSCALL_ERRNO (*ret));
      *ret = -1;
    }
  r = true;

out:
  WRITE_ONCE (ti->getrandom_buf, state);
  return r;
}

static void
reset_stack_list (list_t *list)
{
  struct tls_internal_t *ti = __glibc_tls_internal ();

  list_t *runp;
  list_for_each (runp, &GL (dl_stack_used))
    {
      struct pthread *t = list_entry (runp, struct pthread, list);
      if (&t->tls_state == ti || t->tls_state.getrandom_buf == NULL)
	continue;
      grnd_allocator.states[grnd_allocator.len++]
	= UNMASK_VALUE (t->tls_state.getrandom_buf);
    }
}
#endif

/* Restart the allocator back on a consistent state after fork().  If
   RESET_STATES is true, also put the used getrandom states from kernel back
   to free list.  Otherwise, only make the current state consistent.  */
void
__getrandom_fork_subprocess (bool reset_states)
{
#ifdef HAVE_GETRANDOM_VSYSCALL
  grnd_allocator.lock = LLL_LOCK_INITIALIZER;
  if (grnd_allocator.states == NULL)
    return;

  if (reset_states)
    {
      reset_stack_list (&GL (dl_stack_used));
      reset_stack_list (&GL (dl_stack_user));
    }
  else
    {
      struct tls_internal_t *ti = __glibc_tls_internal ();
      if (ti->getrandom_buf != NULL)
	ti->getrandom_buf = UNMASK_VALUE (ti->getrandom_buf);
    }
#endif
}

ssize_t
__getrandom_nocancel (void *buffer, size_t length, unsigned int flags)
{
#ifdef HAVE_GETRANDOM_VSYSCALL
  ssize_t r;
  if (__getrandom_internal (&r, buffer, length, flags))
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
  if (__getrandom_internal (&r, buffer, length, flags))
    return r;
#endif

  return SYSCALL_CANCEL (getrandom, buffer, length, flags);
}
libc_hidden_def (__getrandom)
weak_alias (__getrandom, getrandom)

void
__getrandom_vdso_release (void)
{
#ifdef HAVE_GETRANDOM_VSYSCALL
  void *state = __glibc_tls_internal()->getrandom_buf;
  if (grnd_allocator.states == NULL || state == NULL)
    return;

  __libc_lock_lock (grnd_allocator.lock);
  grnd_allocator.states[grnd_allocator.len++] = state;
  __libc_lock_unlock (grnd_allocator.lock);
#endif
}
