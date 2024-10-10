/* Return a signal from string describing signal abbreviation.
   Copyright (C) 2024 Free Software Foundation, Inc.
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
#include <ctype.h>
#include <signal.h>
#include <string.h>

static int str2signum (const char *signame)
{
  if (isdigit (*signame))
    {
      char *endp;
      long int n = strtol (signame, &endp, 10);
      if (*endp == '\0' && n < NSIG)
	return n;
    }
  else
    {
      for (int i = 0; i < array_length (__sys_sigabbrev); i++)
	if (__sys_sigabbrev[i] != NULL
	    && strcmp (__sys_sigabbrev[i], signame) == 0)
	  return i;

      enum
	{
	  rtminlen = sizeof ("RTMIN") - 1,
	  rtmaxlen = sizeof ("RTMAX") - 1,
	};

      if (strncmp (signame, "RTMIN", rtminlen) == 0)
	{
	  char *endp;
	  long int n = strtol (signame + rtminlen, &endp, 10);
	  if (*endp == '\0' && n >= 0 && n <= SIGRTMAX - SIGRTMIN)
	    return SIGRTMIN + n;
	}
      else if (strncmp (signame, "RTMAX", sizeof ("RTMAX") - 1) == 0)
	{
	  char *endp;
	  long int n = strtol (signame + rtmaxlen, &endp, 10);
	  if (*endp == '\0' && SIGRTMIN - SIGRTMAX <= n && n <= 0)
	    return SIGRTMAX + n;
	}
    }

  return -1;
}

int
__str2sig (const char *signame, int *signum)
{
  *signum = str2signum (signame);
  return *signum < 0 ? -1 : 0;
}
weak_alias (__str2sig, str2sig)
