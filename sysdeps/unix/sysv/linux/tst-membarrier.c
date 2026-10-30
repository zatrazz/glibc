/* Basic tests for Linux membarrier.
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
#include <sched.h>
#include <stdio.h>
#include <support/check.h>
#include <support/xthread.h>
#include <sys/membarrier.h>

/* MEMBARRIER_CMD_GET_REGISTRATIONS, which kernel headers older than Linux
   6.3 do not define.  */
#define CMD_GET_REGISTRATIONS (1 << 9)

static pthread_barrier_t barrier;

static void *
thread_func (void *closure)
{
  xpthread_barrier_wait (&barrier);
  return NULL;
}

static int
do_test (void)
{
  int mask = membarrier (MEMBARRIER_CMD_QUERY, 0, 0);
  if (mask == -1 && errno == ENOSYS)
    FAIL_UNSUPPORTED ("membarrier is not supported by the kernel");
  TEST_VERIFY_EXIT (mask >= 0);
  printf ("info: supported commands: 0x%x\n", mask);

  /* Flags are only accepted by MEMBARRIER_CMD_PRIVATE_EXPEDITED_RSEQ.  */
  TEST_COMPARE (membarrier (MEMBARRIER_CMD_QUERY, MEMBARRIER_CMD_FLAG_CPU, 0),
		-1);
  TEST_COMPARE (errno, EINVAL);

  TEST_COMPARE (membarrier (CMD_GET_REGISTRATIONS << 1, 0, 0), -1);
  TEST_COMPARE (errno, EINVAL);

  if (mask & MEMBARRIER_CMD_GLOBAL)
    TEST_COMPARE (membarrier (MEMBARRIER_CMD_GLOBAL, 0, 0), 0);

  /* Keep a second thread alive, so the private expedited commands are not
     handled as single-threaded processes.  */
  xpthread_barrier_init (&barrier, NULL, 2);
  pthread_t thr = xpthread_create (NULL, thread_func, NULL);

  if (mask & MEMBARRIER_CMD_PRIVATE_EXPEDITED)
    {
      /* The process must register its intent first.  */
      TEST_COMPARE (membarrier (MEMBARRIER_CMD_PRIVATE_EXPEDITED, 0, 0), -1);
      TEST_COMPARE (errno, EPERM);

      TEST_COMPARE (membarrier (MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED,
				0, 0), 0);
      TEST_COMPARE (membarrier (MEMBARRIER_CMD_PRIVATE_EXPEDITED, 0, 0), 0);

      if (mask & CMD_GET_REGISTRATIONS)
	{
	  int regs = membarrier (CMD_GET_REGISTRATIONS, 0, 0);
	  TEST_VERIFY (regs >= 0);
	  TEST_VERIFY (regs & MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED);
	}
    }

  if (mask & MEMBARRIER_CMD_PRIVATE_EXPEDITED_RSEQ)
    {
      TEST_COMPARE (membarrier (MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED_RSEQ,
				0, 0), 0);
      TEST_COMPARE (membarrier (MEMBARRIER_CMD_PRIVATE_EXPEDITED_RSEQ, 0, 0),
		    0);

      /* Restrict the barrier to the current CPU.  */
      int cpu = sched_getcpu ();
      if (cpu < 0)
	cpu = 0;
      TEST_COMPARE (membarrier (MEMBARRIER_CMD_PRIVATE_EXPEDITED_RSEQ,
				MEMBARRIER_CMD_FLAG_CPU, cpu), 0);

      TEST_COMPARE (membarrier (MEMBARRIER_CMD_PRIVATE_EXPEDITED_RSEQ,
				MEMBARRIER_CMD_FLAG_CPU << 1, cpu), -1);
      TEST_COMPARE (errno, EINVAL);
    }

  xpthread_barrier_wait (&barrier);
  xpthread_join (thr);
  xpthread_barrier_destroy (&barrier);

  return 0;
}

#include <support/test-driver.c>
