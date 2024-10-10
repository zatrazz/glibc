/* Return string describing signal abbreviation.
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

#include <string.h>
#include <signal.h>
#include <stdlib.h>

int
__sig2str (int signum, char *str)
{
  const char *sigabbrev = __sigabbrev_np (signum);
  if (sigabbrev != NULL)
    {
      strcpy (str, sigabbrev);
      return 0;
    }

  if (signum < SIGRTMIN || signum > SIGRTMAX)
    return -1;

  int base;
  if (signum <= SIGRTMIN + (SIGRTMAX - SIGRTMIN) / 2)
    {
      str = stpcpy (str, "RTMIN");
      base = SIGRTMIN;
    }
  else
    {
      str = stpcpy (str, "RTMAX");
      base = SIGRTMAX;
    }

  int delta = signum - base;
  if (delta != 0)
    {
      str[0] = delta > 0 ? '+' : '-';
      delta = abs (delta);

      unsigned int i = 1;
      for (unsigned int j = delta; j != 0; j /= 10, i++);
      str[i] = '\0';

      for (; delta != 0; delta /= 10)
	str[--i] = '0' + delta % 10;
    }

  return 0;
}
weak_alias (__sig2str, sig2str)
