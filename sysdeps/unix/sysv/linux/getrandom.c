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

#ifdef __NR_vgetrandom_alloc
# include <signal-reentrancy.h>

static struct
{
  __libc_lock_define (, lock);
  void **states;
  size_t len;
  size_t cap;
  size_t size_per_each;
} grnd_allocator =
{
  .lock = LLL_LOCK_INITIALIZER
};

static void *
vgetrandom_alloc_syscall (unsigned int *num, unsigned int *size_per_each,
                          void *addr, unsigned int flags)
{
  long int r = INTERNAL_SYSCALL_CALL (vgetrandom_alloc, num, size_per_each, addr, flags);
  return INTERNAL_SYSCALL_ERROR_P(r) ? NULL : (void *) r;
}

static struct grnd_allocator_state *
vgetrandom_get_state (void)
{
  void *state = NULL;

  __libc_lock_lock (grnd_allocator.lock);

  if (grnd_allocator.len == 0)
    {
      size_t new_cap;
      unsigned int num = __get_nprocs(); /* Just a decent heuristic. */
      unsigned int size_per_each = 0; /* Must be zero on input. */

      void *new_block = vgetrandom_alloc_syscall (&num, &size_per_each, NULL, 0);
      if (new_block == NULL)
        goto out;

      if (grnd_allocator.size_per_each && grnd_allocator.size_per_each != size_per_each)
        goto unmap;
      grnd_allocator.size_per_each = size_per_each;

      new_cap = grnd_allocator.cap + num;
      void *new_states = __libc_reallocarray (grnd_allocator.states, new_cap,
        sizeof (*grnd_allocator.states));
      if (new_states == NULL)
        goto unmap;

      grnd_allocator.cap = new_cap;
      grnd_allocator.states = new_states;

      for (size_t i = 0; i < num; ++i)
        {
          grnd_allocator.states[i] = new_block;
          if (((uintptr_t)new_block & (GLRO(dl_pagesize) - 1)) + size_per_each
			  > GLRO(dl_pagesize))
	    new_block = PTR_ALIGN_DOWN (new_block + size_per_each,
					GLRO(dl_pagesize));
          else
            new_block += size_per_each;
        }
      grnd_allocator.len = num;
      goto success;

    unmap:
      __munmap (new_block, howmany (num, GLRO(dl_pagesize) / size_per_each)
			   * GLRO(dl_pagesize));
      goto out;
    }

success:
  state = grnd_allocator.states[--grnd_allocator.len];

out:
  __libc_lock_unlock (grnd_allocator.lock);
  return state;
}

/* Return true if the syscall fallback should be issued in the case the vDSO
   is not present, in the case of reentrancy, or if any memory allocation fails.  */
static bool
__getrandom_internal (ssize_t *ret, void *buffer, size_t length,
                      unsigned int flags)
{
  if (GLRO(dl_vdso_getrandom) == NULL)
    return false;

  struct tls_internal_t *ti = __glibc_tls_internal ();
  void *state;
  if (!signal_exchange_value (&ti->getrandom_buf, &state))
    return false;

  bool r = false;
  if (state == NULL)
    {
      state = vgetrandom_get_state ();
      if (state == NULL)
        goto out;
    }

  *ret = GLRO(dl_vdso_getrandom)(buffer, length, flags, state, grnd_allocator.size_per_each);
  if (INTERNAL_SYSCALL_ERROR_P (*ret))
    {
      __set_errno (INTERNAL_SYSCALL_ERRNO (*ret));
      *ret = -1;
    }
  r = true;

out:
  signal_store_value (&ti->getrandom_buf, state);
  return r;
}
#endif

ssize_t
__getrandom_nocancel (void *buffer, size_t length, unsigned int flags)
{
#ifdef __NR_vgetrandom_alloc
  ssize_t r;
  if (__getrandom_internal (&r, buffer, length, flags))
    return r;
#endif

  return SYSCALL_CANCEL (getrandom, buffer, length, flags);
}

/* Write up to LENGTH bytes of randomness starting at BUFFER.
   Return the number of bytes written, or -1 on error.  */
ssize_t
__getrandom (void *buffer, size_t length, unsigned int flags)
{
#ifdef __NR_vgetrandom_alloc
  ssize_t r;
  if (__getrandom_internal (&r, buffer, length, flags))
    return r;
#endif

  return INTERNAL_SYSCALL_CALL (getrandom, buffer, length, flags);
}
libc_hidden_def (__getrandom)
weak_alias (__getrandom, getrandom)

void
__getrandom_vdso_release (void)
{
#ifdef __NR_vgetrandom_alloc
  void *state = __glibc_tls_internal ()->getrandom_buf;
  if (state == NULL)
    return;

  __libc_lock_lock (grnd_allocator.lock);
  if (grnd_allocator.states != NULL)
    grnd_allocator.states[grnd_allocator.len++] = state;
  __libc_lock_unlock (grnd_allocator.lock);
#endif
}
