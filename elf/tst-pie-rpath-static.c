/* Check if RPATH/RUNPATH is allowed for static-pie.
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

#include <stdlib.h>
#include <support/xunistd.h>
#include <support/support.h>
#include <support/xdlfcn.h>
#include <support/check.h>
#include <support/temp_file.h>

#define LIBNAME     "tst-pie-rpath-mod.so"
#define TESTSUBDIR  PFX "tst-pie-rpath-static-subdir"
#define LIBPATH     TESTSUBDIR "/" LIBNAME

static void
do_prepare (int argc, char **argv)
{
  xmkdir (TESTSUBDIR, 0777);
  add_temp_file (TESTSUBDIR);

  support_copy_file (PFX "/" LIBNAME, LIBPATH);
  add_temp_file (LIBPATH);
}
#define PREPARE do_prepare

static int
do_test (void)
{
  void *h = xdlopen (LIBNAME, RTLD_NOW);
  int (*foo)(void) = xdlsym (h, "foo");
  TEST_COMPARE (foo (), 42);
  xdlclose (h);

  return 0;
}

#include <support/test-driver.c>
