/* Test the tunables set by glibc.rtld.hardened.
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

#include <dl-hardened.h>
#include <stdint.h>
#include <support/check.h>

#include "dl-tunables.h"

/* tst-tunables-hardened runs with glibc.rtld.hardened=1, and
   tst-tunables-hardened-override also sets glibc.rtld.textrel=1 and
   glibc.rtld.relro=0, which take precedence.  */
#ifndef OVERRIDE
# define OVERRIDE 0
#endif

#define GET(ns, id) TUNABLE_GET_FULL (glibc, ns, id, int32_t, NULL)

static int
do_test (void)
{
  /* Reaching this point also means that the settings accept the test.  */
  TEST_COMPARE (GET (rtld, hardened), 1);
  TEST_COMPARE (GET (rtld, require_pie), 1);
  TEST_COMPARE (GET (rtld, execstack), DL_HARDENED_SKIP_EXECSTACK ? 1 : 0);
  TEST_COMPARE (GET (rtld, textrel), OVERRIDE ? 1 : 0);
  TEST_COMPARE (GET (rtld, wx_segments), DL_HARDENED_SKIP_WX_SEGMENTS ? 1 : 0);
  TEST_COMPARE (GET (rtld, bind_now), 1);
  TEST_COMPARE (GET (rtld, relative_rpath), 0);
  TEST_COMPARE (GET (rtld, relro), OVERRIDE ? 0 : 1);

  return 0;
}

#include <support/test-driver.c>
