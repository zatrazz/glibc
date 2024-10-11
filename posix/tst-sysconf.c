/* Basic tests sysconf.g
   Copyright (C) XXXX-2024 Free Software Foundation, Inc.
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
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <support/check.h>

struct sysconf_test_t
{
  long int _P_val;
  const char *name;
  int _SC_val;
  bool positive;
  bool posix2;
};

static struct sysconf_test_t posix_options[] =
  {
#define N_(name, pos) { _POSIX_##name, #name, _SC_##name, pos, false }
#define NP(name) N_ (name, true)
#define N(name) N_ (name, false)
#define N2(name) { _POSIX2_##name, #name, _SC_2_##name, false, true }
    N (ADVISORY_INFO),
    N (ASYNCHRONOUS_IO),
    N (BARRIERS),
    N (CLOCK_SELECTION),
    N (CPUTIME),
    N (FSYNC),
    N (IPV6),
    NP (JOB_CONTROL),
    N (MAPPED_FILES),
    N (MEMLOCK),
    N (MEMLOCK_RANGE),
    N (MEMORY_PROTECTION),
    N (MESSAGE_PASSING),
    N (MONOTONIC_CLOCK),
#ifdef _POSIX_PRIORITIZED_IO
    N (PRIORITIZED_IO),
#endif
#ifdef _POSIX_PRIORITY_SCHEDULING
    N (PRIORITY_SCHEDULING),
#endif
    N (RAW_SOCKETS),
    N (READER_WRITER_LOCKS),
    N (REALTIME_SIGNALS),
    NP (REGEXP),
    NP (SAVED_IDS),
    N (SEMAPHORES),
    N (SHARED_MEMORY_OBJECTS),
    NP (SHELL),
    N (SPAWN),
    N (SPIN_LOCKS),
    N (SPORADIC_SERVER),
#ifdef _POSIX_SYNCHRONIZED_IO
    N (SYNCHRONIZED_IO),
#endif
    N (THREAD_ATTR_STACKADDR),
    N (THREAD_ATTR_STACKSIZE),
    N (THREAD_CPUTIME),
    N (THREAD_PRIO_INHERIT),
    N (THREAD_PRIO_PROTECT),
    N (THREAD_PRIORITY_SCHEDULING),
    N (THREAD_PROCESS_SHARED),
    N (THREAD_SAFE_FUNCTIONS),
    N (THREAD_SPORADIC_SERVER),
    N (THREADS),
    N (TIMEOUTS),
    N (TIMERS),
    N (TRACE),
    N (TRACE_EVENT_FILTER),
    N (TRACE_INHERIT),
    N (TRACE_LOG),
    N (TYPED_MEMORY_OBJECTS),
    N2 (C_BIND),
    N2 (C_DEV),
    N2 (CHAR_TERM),
    { NSIG, "NSIG", _SC_NSIG, true, true},
  };

static int
do_test (void)
{
  for (int i = 0; i < array_length (posix_options); ++i)
    {
      long int scret = sysconf (posix_options[i]._SC_val);

      if (scret == 0)
	{
	  printf ("sysconf(_SC_%s%s) returned zero\n",
		  posix_options[i].posix2 ? "2_" : "", posix_options[i].name);
	  support_record_failure ();
	}
      if (posix_options[i]._P_val != 0 && posix_options[i]._P_val != scret)
	{
	  printf ("sysconf(_SC_%s%s) = %ld does not match _POSIX%s_%s = %ld\n",
		  posix_options[i].posix2 ? "2_" : "", posix_options[i].name,
		  scret,
		  posix_options[i].posix2 ? "2" : "", posix_options[i].name,
		  posix_options[i]._P_val);
	  support_record_failure ();
	}
      else if (posix_options[i].positive && scret < 0)
	{
	  printf ("sysconf(_SC_%s%s) must be > 0\n",
		  posix_options[i].posix2 ? "2_" : "", posix_options[i].name);
	  support_record_failure ();
	}

#define STDVER 200809L
      if (scret > 0 && scret != STDVER && !posix_options[i].positive)
	{
	  printf ("sysconf(_SC_%s%s) must be %ldL\n",
		  posix_options[i].posix2 ? "2_" : "", posix_options[i].name,
		  STDVER);
	  support_record_failure ();
	}
    }

  return 0;
}

#include <support/test-driver.c>
