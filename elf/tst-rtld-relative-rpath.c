/* Test the glibc.rtld.relative_rpath tunable.
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
#include <stdlib.h>
#include <support/check.h>
#include <support/support.h>
#include <support/temp_file.h>
#include <support/xdlfcn.h>
#include <support/xunistd.h>

/* tst-rtld-relative-rpath runs with glibc.rtld.relative_rpath=0, and
   tst-rtld-relative-rpath-default with the default value.  */
#ifndef RELATIVE_ALLOWED
# define RELATIVE_ALLOWED 0
#endif

/* tst-rtld-relative-rpath-mod.so has the DT_RUNPATH
   "tst-rtld-relative-rpath-dir::/nonexistent", a relative entry and an
   empty one, both resolved against the current directory.  Creates a
   symbolic link to tst-rtld-relative-rpath-lib.so for each of them in a
   emporary directory, which becomes the current directory:

     RELATIVE_DIR/LIB_RELATIVE   found through the relative entry
     LIB_EMPTY                   found through the empty entry

   Neither name exists in any other search directory, so each dlopen
   succeeds only if its entry is used.  */
#define RELATIVE_DIR  "tst-rtld-relative-rpath-dir"
#define LIB_RELATIVE  "libtst-rtld-relative-rpath-relative.so"
#define LIB_EMPTY     "libtst-rtld-relative-rpath-empty.so"

static void
prepare (int argc, char **argv)
{
  char *dir = support_create_temp_directory ("tst-rtld-relative-rpath-");
  char *lib = xasprintf ("%s/elf/tst-rtld-relative-rpath-lib.so",
			 support_objdir_root);

  char *reldir = xasprintf ("%s/%s", dir, RELATIVE_DIR);
  xmkdir (reldir, 0700);
  add_temp_file (reldir);
  char *librel = xasprintf ("%s/%s", reldir, LIB_RELATIVE);
  xsymlink (lib, librel);
  add_temp_file (librel);
  char *libempty = xasprintf ("%s/%s", dir, LIB_EMPTY);
  xsymlink (lib, libempty);
  add_temp_file (libempty);

  xchdir (dir);

  free (libempty);
  free (librel);
  free (reldir);
  free (lib);
  free (dir);
}

static int
do_test (void)
{
  void *mod = xdlopen ("tst-rtld-relative-rpath-mod.so", RTLD_NOW);
  void *(*open_lib) (const char *)
    = xdlsym (mod, "tst_rtld_relative_rpath_open");

  static const char *const names[] = { LIB_RELATIVE, LIB_EMPTY };
  for (size_t i = 0; i < array_length (names); i++)
    {
      void *h = open_lib (names[i]);
#if RELATIVE_ALLOWED
      if (h == NULL)
	FAIL_EXIT1 ("dlopen (\"%s\"): %s", names[i], dlerror ());
      int (*value) (void) = xdlsym (h, "tst_rtld_relative_rpath_value");
      TEST_COMPARE (value (), 42);
      xdlclose (h);
#else
      if (h != NULL)
	FAIL_EXIT1 ("dlopen (\"%s\") succeeded", names[i]);
      printf ("info: dlopen (\"%s\"): %s\n", names[i], dlerror ());
#endif
    }

  xdlclose (mod);

  return 0;
}

#define PREPARE prepare
#include <support/test-driver.c>
