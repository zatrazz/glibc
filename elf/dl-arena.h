/* Loader-internal arena (bump-pointer) allocator.
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

/* Bump-pointer arena allocator for transient loader allocations.
   Allocations come from a 256-byte inline stack buffer first; overflow
   pages are obtained from mmap (early startup, before real malloc is
   active) or malloc (later).  All pages are freed in one shot by
   dl_arena_free.

   On allocation failure _dl_arena_alloc_slow raises a loader ENOMEM
   via _dl_signal_error and does not return.

   Typical usage:

     struct dl_arena arena;
     dl_arena_init (&arena);
     void *p = dl_arena_alloc (&arena, n);
     ... use p ...
     dl_arena_free (&arena);

   To guarantee cleanup even on error, wrap the work in a function
   called via _dl_catch_exception; call dl_arena_free unconditionally
   after _dl_catch_exception returns.  */

#ifndef _DL_ARENA_H
#define _DL_ARENA_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <libc-pointer-arith.h>

/* Size of the inline buffer.  Chosen to cover the initial BFS 'known'
   node array for a typical startup (executable + a handful of preloads)
   without any system call or malloc.  */
enum { DL_ARENA_INLINE_SIZE = 256 };

/* Minimum size of an overflow page allocated from mmap or malloc.  */
enum { DL_ARENA_PAGE_MIN = 4096 };

struct dl_arena_page
{
  struct dl_arena_page *next;   /* older page in the allocation chain */
  size_t size;                   /* total size of this allocation */
  bool use_malloc;               /* free with free(), else __munmap() */
};

struct dl_arena
{
  char *bump;                    /* next free byte */
  char *limit;                   /* one past the end of the current page */
  struct dl_arena_page *pages;   /* chain of heap pages to free */
  char inline_data[DL_ARENA_INLINE_SIZE]
    __attribute__ ((aligned (__alignof__ (max_align_t))));
};

/* Slow path: allocate a new overflow page and return SIZE bytes from it.
   Never returns NULL -- raises a loader ENOMEM on failure.  */
extern void *_dl_arena_alloc_slow (struct dl_arena *a, size_t size)
  __nonnull ((1)) attribute_hidden;
rtld_hidden_proto (_dl_arena_alloc_slow)

/* Free all overflow pages and reset the arena to its inline buffer.  */
extern void dl_arena_free (struct dl_arena *a)
  __nonnull ((1)) attribute_hidden;
rtld_hidden_proto (dl_arena_free)

/* Initialize ARENA to use its inline buffer.  */
static __always_inline void
dl_arena_init (struct dl_arena *a)
{
  a->bump = a->inline_data;
  a->limit = a->inline_data + sizeof a->inline_data;
  a->pages = NULL;
}

/* Allocate SIZE bytes from ARENA, aligned to max_align_t.
   Never returns NULL -- raises a loader ENOMEM on failure.  */
static __always_inline __attribute_warn_unused_result__ void *
dl_arena_alloc (struct dl_arena *a, size_t size)
{
  uintptr_t ptr = ALIGN_UP ((uintptr_t) a->bump, __alignof__ (max_align_t));
  if (__glibc_likely (ptr + size <= (uintptr_t) a->limit))
    {
      a->bump = (char *) (ptr + size);
      return (void *) ptr;
    }
  return _dl_arena_alloc_slow (a, size);
}

#endif /* dl-arena.h */
