/* Loader-internal arena (bump-pointer) allocator -- overflow and free paths.
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

#include <dl-arena.h>

#include <errno.h>
#include <ldsodefs.h>
#include <libintl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/param.h>

void *
_dl_arena_alloc_slow (struct dl_arena *a, size_t size)
{
  size_t need = sizeof (struct dl_arena_page) + __alignof__ (max_align_t) + size;
  size_t page_size = MAX (need, (size_t) DL_ARENA_PAGE_MIN);
  struct dl_arena_page *page;
  bool use_malloc = false;
#ifdef SHARED
  use_malloc = __rtld_malloc_is_complete ();
#endif

  if (use_malloc)
    {
      page = malloc (page_size);
      if (__glibc_unlikely (page == NULL))
	_dl_signal_error (ENOMEM, NULL, NULL,
			  N_("cannot allocate dependency list node"));
    }
  else
    {
      page_size = ALIGN_UP (page_size, GLRO (dl_pagesize));
      page = __mmap (NULL, page_size, PROT_READ | PROT_WRITE,
		     MAP_ANON | MAP_PRIVATE, -1, 0);
      if (__glibc_unlikely (page == MAP_FAILED))
	_dl_signal_error (ENOMEM, NULL, NULL,
			  N_("cannot allocate dependency list node"));
    }

  page->next = a->pages;
  page->size = page_size;
  page->use_malloc = use_malloc;
  a->pages = page;

  char *data = (char *) ALIGN_UP ((uintptr_t) (page + 1),
				   __alignof__ (max_align_t));
  a->bump = data + size;
  a->limit = (char *) page + page_size;
  return data;
}
rtld_hidden_def (_dl_arena_alloc_slow)

void
dl_arena_free (struct dl_arena *a)
{
  struct dl_arena_page *page = a->pages;
  while (page != NULL)
    {
      struct dl_arena_page *next = page->next;
      if (page->use_malloc)
	free (page);
      else
	__munmap (page, page->size);
      page = next;
    }
  a->pages = NULL;
  a->bump = a->inline_data;
  a->limit = a->inline_data + sizeof a->inline_data;
}
rtld_hidden_def (dl_arena_free)
