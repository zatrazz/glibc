/* Basic tests for Linux ioprio_get and ioprio_set.
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

#include <errno.h>
#include <sys/ioprio.h>
#include <unistd.h>
#include <support/check.h>

static int
do_test (void)
{
  int prio = ioprio_get (IOPRIO_WHO_PROCESS, 0);
  if (prio == -1 && errno == ENOSYS)
    FAIL_UNSUPPORTED ("ioprio_get is not supported by the kernel");
  TEST_VERIFY_EXIT (prio >= 0);

  /* An unprivileged process can use the best-effort and idle classes.  */
  prio = IOPRIO_PRIO_VALUE (IOPRIO_CLASS_BE, IOPRIO_BE_NR - 1);
  TEST_COMPARE (IOPRIO_PRIO_CLASS (prio), IOPRIO_CLASS_BE);
  TEST_COMPARE (IOPRIO_PRIO_DATA (prio), IOPRIO_BE_NR - 1);
  TEST_COMPARE (ioprio_set (IOPRIO_WHO_PROCESS, 0, prio), 0);
  TEST_COMPARE (ioprio_get (IOPRIO_WHO_PROCESS, 0), prio);
  TEST_COMPARE (ioprio_get (IOPRIO_WHO_PROCESS, getpid ()), prio);

  prio = IOPRIO_PRIO_VALUE (IOPRIO_CLASS_IDLE, 0);
  TEST_COMPARE (ioprio_set (IOPRIO_WHO_PROCESS, getpid (), prio), 0);
  TEST_COMPARE (IOPRIO_PRIO_CLASS (ioprio_get (IOPRIO_WHO_PROCESS, 0)),
		IOPRIO_CLASS_IDLE);

  /* Invalid WHICH argument.  */
  TEST_COMPARE (ioprio_get (IOPRIO_WHO_USER + 1, 0), -1);
  TEST_COMPARE (errno, EINVAL);

  /* Invalid class.  */
  TEST_COMPARE (ioprio_set (IOPRIO_WHO_PROCESS, 0,
			    (IOPRIO_CLASS_IDLE + 2) << IOPRIO_CLASS_SHIFT), -1);
  TEST_COMPARE (errno, EINVAL);

  return 0;
}

#include <support/test-driver.c>
