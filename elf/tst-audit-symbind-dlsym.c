/* Check la_symbind dispatch for dlsym with multiple auditors.
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
#include <support/check.h>
#include <support/xdlfcn.h>

static int
do_test (void)
{
  void *h = xdlopen ("tst-audit-symbind-dlsym-mod.so", RTLD_NOW);
  const int *p = xdlsym (h, "tst_audit_symbind_dlsym_data");
  /* 1 means only the second auditor was called; 2 means the first
     one was called instead.  */
  TEST_COMPARE (*p, 1);
  xdlclose (h);
  return 0;
}

#include <support/test-driver.c>
