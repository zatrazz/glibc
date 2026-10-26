/* Test the glibc.rtld.bind_now tunable.
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
#include <stdio.h>
#include <string.h>
#include <support/check.h>
#include <support/xdlfcn.h>

/* tst-rtld-bind-now runs with glibc.rtld.bind_now=1, and
   tst-rtld-bind-now-default with the default value.
   tst-rtld-bind-now-profile also sets LD_PROFILE, which requires lazy
   binding.  */
#ifndef LAZY_BINDING
# define LAZY_BINDING 0
#endif

#define MODULE "tst-rtld-bind-now-mod.so"

static int
do_test (void)
{
#if LAZY_BINDING
  /* The undefined symbol is not bound until it is called.  */
  void *h = xdlopen (MODULE, RTLD_LAZY);
  int (*value) (void) = xdlsym (h, "tst_rtld_bind_now_value");
  TEST_COMPARE (value (), 42);
  xdlclose (h);
#else
  /* The tunable makes RTLD_LAZY behave as RTLD_NOW.  */
  void *h = dlopen (MODULE, RTLD_LAZY);
  TEST_VERIFY (h == NULL);
  const char *err = dlerror ();
  TEST_VERIFY_EXIT (err != NULL);
  printf ("info: dlopen error: %s\n", err);
  TEST_VERIFY (strstr (err, "undefined symbol: tst_rtld_bind_now_undefined")
	       != NULL);
#endif

  return 0;
}

#include <support/test-driver.c>
