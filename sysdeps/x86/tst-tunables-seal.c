/* Verify that string tunables are sealed after early startup.
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

/* This test is started with GLIBC_TUNABLES=glibc.cpu.hwcaps=-AVX2 in the
   environment (see the Makefile).  The string tunable is therefore set and
   is consumed by init_cpu_features during early startup.  By the time
   do_test runs the tunables have been sealed, so the reference into the
   environment block has been dropped.

   Reading a sealed string tunable must never hand back a reference into the
   application-owned environment block: __tunable_get_val aborts when
   assertions are enabled (the common case for the loader), or returns NULL
   otherwise.  The read is therefore done in a subprocess.  */

#define TUNABLE_NAMESPACE cpu
#include <elf/dl-tunables.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <support/capture_subprocess.h>
#include <support/check.h>

static void
read_sealed_tunable (void *closure)
{
  const struct tunable_str_t *hwcaps
    = TUNABLE_GET (hwcaps, struct tunable_str_t *, NULL);
  /* Reaching here means assertions are disabled; the value must have been
     dropped (NULL) rather than still aliasing the environment block.  */
  _exit (hwcaps == NULL ? 0 : 2);
}

static int
do_test (void)
{
  struct support_capture_subprocess result
    = support_capture_subprocess (read_sealed_tunable, NULL);

  if (WIFEXITED (result.status))
    {
      int code = WEXITSTATUS (result.status);
      /* 0:   assertions disabled, the read returned NULL.
	 127: the loader assertion fired and aborted startup.
	 A live reference leaking back (exit 2) is the failure.  */
      TEST_VERIFY (code == 0 || code == 127);
    }
  else
    /* libc-style assert raises SIGABRT.  */
    TEST_COMPARE (WTERMSIG (result.status), SIGABRT);

  support_capture_subprocess_free (&result);
  return 0;
}

#include <support/test-driver.c>
