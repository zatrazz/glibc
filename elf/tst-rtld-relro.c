/* Test the glibc.rtld.relro tunable with dlopen.
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

#include <array_length.h>
#include <dlfcn.h>
#include <stdio.h>
#include <support/xdlfcn.h>

#include "tst-rtld-relro.h"

static const struct
{
  const char *name;
  bool relro;
  bool now;
} modules[] =
{
  { "tst-rtld-relro-mod-norelro.so", false, false },
  { "tst-rtld-relro-mod-lazy.so", true, false },
  { "tst-rtld-relro-mod-now.so", true, true },
};

/* Check dlopen of the modules with glibc.rtld.relro set to LEVEL.  */
static void
check_modules (int level)
{
  for (size_t i = 0; i < array_length (modules); i++)
    {
      const char *name = modules[i].name;
      /* The lowest glibc.rtld.relro value that rejects the module.  */
      int reject = !modules[i].relro ? 1 : !modules[i].now ? 2 : 3;

      void *h = dlopen (name, RTLD_NOW);
      if (level >= reject)
	{
	  if (h != NULL)
	    FAIL_EXIT1 ("dlopen (\"%s\") succeeded", name);
	  const char *err = dlerror ();
	  printf ("info: dlopen (\"%s\"): %s\n", name, err);
	  TEST_VERIFY (strstr (err, reject == 1
				    ? ": RELRO is required"
				    : ": full RELRO is required") != NULL);
	}
      else
	{
	  if (h == NULL)
	    FAIL_EXIT1 ("dlopen (\"%s\"): %s", name, dlerror ());

	  /* Check that the module was linked as expected.  */
	  struct link_map *l;
	  TEST_COMPARE (dlinfo (h, RTLD_DI_LINKMAP, &l), 0);
	  check_relro_info (l->l_name, modules[i].relro, modules[i].now);

	  int (*value) (void) = xdlsym (h, "tst_rtld_relro_value");
	  TEST_COMPARE (value (), 42);
	  xdlclose (h);
	}
    }
}

/* tst-rtld-relro-full includes this file for check_modules.  */
#ifndef RELRO_CHECK_MODULES_ONLY

/* The value of glibc.rtld.relro: tst-rtld-relro-default runs with 0, and
   tst-rtld-relro with 1.  */
# ifndef RELRO_LEVEL
#  define RELRO_LEVEL 1
# endif

static int
do_test (void)
{
  check_modules (RELRO_LEVEL);
  return 0;
}

# include <support/test-driver.c>
#endif
