/* Check for explicit vDSO dependency (BZ 33335)
   Copyright (C) 2025 Free Software Foundation, Inc.
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

#include <time.h>
#include <support/check.h>

extern int __kernel_clock_gettime(clockid_t clk_id, struct timespec *tp);

static int
do_test (void)
{
  TEST_COMPARE (__kernel_clock_gettime (CLOCK_REALTIME,
					&(struct timespec) { 0 }),
		42);

  return 0;
}

#include <support/test-driver.c>
