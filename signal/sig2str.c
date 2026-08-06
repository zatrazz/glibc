/* Translate a signal number to a signal name.
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

#include <_itoa.h>
#include <array_length.h>
#include <signal.h>
#include <string.h>

int
sig2str (int signum, char *str)
{
  int rtmin = SIGRTMIN;
  int rtmax = SIGRTMAX;

  if (signum <= 0 || signum > rtmax)
    return -1;

  /* The longest possible output is "RTMIN+" followed by the decimal
     representation of an int, which fits comfortably in SIG2STR_MAX
     (32) bytes.  */
  if (signum < array_length (__sys_sigabbrev)
      && __sys_sigabbrev[signum] != NULL)
    strcpy (str, __sys_sigabbrev[signum]);
  else if (signum < rtmin)
    /* A valid signal without a name, such as the signals reserved for
       the internal NPTL use; use the decimal representation as the
       implementation-defined unique identification.  */
    *_fitoa_word (signum, str, 10, 0) = '\0';
  else if (signum == rtmin)
    strcpy (str, "RTMIN");
  else if (signum == rtmax)
    strcpy (str, "RTMAX");
  else if (signum <= (rtmin + rtmax) / 2)
    {
      char *p = mempcpy (str, "RTMIN+", strlen ("RTMIN+"));
      *_fitoa_word (signum - rtmin, p, 10, 0) = '\0';
    }
  else
    {
      char *p = mempcpy (str, "RTMAX-", strlen ("RTMAX-"));
      *_fitoa_word (rtmax - signum, p, 10, 0) = '\0';
    }

  return 0;
}
