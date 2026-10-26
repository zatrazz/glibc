/* Test the glibc.rtld.textrel tunable.
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

#include <dlfcn.h>
#include <link.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <support/check.h>
#include <support/xdlfcn.h>

/* tst-rtld-textrel runs with glibc.rtld.textrel=0, and
   tst-rtld-textrel-default with the default value.  */
#ifndef TEXTREL_ALLOWED
# define TEXTREL_ALLOWED 0
#endif

#define MODULE "tst-rtld-textrel-mod.so"

#if TEXTREL_ALLOWED
static bool
has_textrel (const struct link_map *l)
{
  for (const ElfW(Dyn) *d = l->l_ld; d->d_tag != DT_NULL; d++)
    if (d->d_tag == DT_TEXTREL
	|| (d->d_tag == DT_FLAGS && (d->d_un.d_val & DF_TEXTREL) != 0))
      return true;
  return false;
}
#endif

static int
do_test (void)
{
#if TEXTREL_ALLOWED
  void *h = xdlopen (MODULE, RTLD_NOW);

  /* Check that the module has a text relocation, so that the failure in
     tst-rtld-textrel is caused by the tunable, and that it was applied.  */
  struct link_map *l;
  TEST_COMPARE (dlinfo (h, RTLD_DI_LINKMAP, &l), 0);
  TEST_VERIFY (has_textrel (l));

  uintptr_t (*value) (void) = xdlsym (h, "tst_rtld_textrel_value");
  const void *(*location) (void) = xdlsym (h, "tst_rtld_textrel_location");
  TEST_COMPARE (value (), (uintptr_t) location ());

  xdlclose (h);
#else
  void *h = dlopen (MODULE, RTLD_NOW);
  TEST_VERIFY (h == NULL);
  const char *err = dlerror ();
  TEST_VERIFY_EXIT (err != NULL);
  printf ("info: dlopen error: %s\n", err);
  TEST_VERIFY (strstr (err, "text relocations are not allowed") != NULL);
#endif

  return 0;
}

#include <support/test-driver.c>
