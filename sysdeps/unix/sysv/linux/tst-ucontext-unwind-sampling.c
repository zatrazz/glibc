/* Asynchronous backtraces while contexts switch (BZ 34575).
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

/* A CPU-time timer interrupts two contexts switching back and forth
   and the handler takes a backtrace, as a profiler would.  Samples land
   anywhere, so this finds bad unwind information only probabilistically,
   but correct unwind information never fails it.  */

#include "tst-ucontext-unwind-common.c"

#include <time.h>

#define MAX_SAMPLES 4000

static void
stop_handler (int sig)
{
  stop = true;
}

static int
sample_mode (enum switch_mode m, const char *what)
{
  int first = nsamples;
  maxsamples = first + MAX_SAMPLES / 2;
  mode = m;
  stop = false;

  /* One second of switching, or enough samples.  */
  timer_t prof, wall;
  struct sigevent sev = { .sigev_notify = SIGEV_SIGNAL,
			  .sigev_signo = SIGPROF };
  if (timer_create (CLOCK_THREAD_CPUTIME_ID, &sev, &prof) != 0)
    FAIL_EXIT1 ("timer_create: %m");
  sev.sigev_signo = SIGALRM;
  if (timer_create (CLOCK_MONOTONIC, &sev, &wall) != 0)
    FAIL_EXIT1 ("timer_create: %m");
  struct itimerspec its = { .it_interval = { 0, 50000 },
			    .it_value = { 0, 50000 } };
  if (timer_settime (prof, 0, &its, NULL) != 0)
    FAIL_EXIT1 ("timer_settime: %m");
  its = (struct itimerspec) { .it_value = { 1, 0 } };
  if (timer_settime (wall, 0, &its, NULL) != 0)
    FAIL_EXIT1 ("timer_settime: %m");

  while (!stop && nsamples < maxsamples)
    tcu_run (1000);

  timer_delete (prof);
  timer_delete (wall);

  int trunc;
  return check_samples (what, first, nsamples, &trunc, false);
}

static int
do_test (void)
{
  init_libc_base ();
  init_samples (MAX_SAMPLES);
  install_handler (SIGPROF, SA_RESTART);
  /* No samples inside the stop handler.  */
  struct sigaction sa = { .sa_handler = stop_handler, .sa_flags = SA_RESTART };
  sigemptyset (&sa.sa_mask);
  sigaddset (&sa.sa_mask, SIGPROF);
  xsigaction (SIGALRM, &sa, NULL);
  init_contexts ();

  TEST_COMPARE (sample_mode (MODE_SWAP, "swapcontext"), 0);
  TEST_COMPARE (sample_mode (MODE_SET, "setcontext"), 0);
  return 0;
}

#include <support/test-driver.c>
