/* Translate a signal name to a signal number.
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

#include <array_length.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

/* Parse the string at STR as a decimal number, strictly: at least one
   digit, only digits, and no overflow.  */
static bool
parse_number (const char *str, int *num)
{
  if (*str < '0' || *str > '9')
    return false;

  int n = 0;
  for (; *str >= '0' && *str <= '9'; str++)
    {
      int digit = *str - '0';
      if (n > (INT_MAX - digit) / 10)
	return false;
      n = n * 10 + digit;
    }
  if (*str != '\0')
    return false;

  *num = n;
  return true;
}

int
str2sig (const char *str, int *pnum)
{
  /* A decimal representation of a supported signal number.  */
  if (*str >= '0' && *str <= '9')
    {
      int n;
      if (!parse_number (str, &n) || n < 1 || n >= NSIG)
	return -1;
      *pnum = n;
      return 0;
    }

  /* RTMIN, RTMAX, and the offset forms such as RTMIN+2 and RTMAX-5.
     The offset may use either sign; the resulting signal number is
     validated against the realtime signal range.  */
  int rtmin = SIGRTMIN;
  int rtmax = SIGRTMAX;
  bool is_min = strncmp (str, "RTMIN", strlen ("RTMIN")) == 0;
  if (is_min || strncmp (str, "RTMAX", strlen ("RTMAX")) == 0)
    {
      int sig = is_min ? rtmin : rtmax;
      const char *rest = str + strlen ("RTMIN");
      if (*rest != '\0')
	{
	  bool negative = *rest == '-';
	  if (!negative && *rest != '+')
	    return -1;
	  int n;
	  if (!parse_number (rest + 1, &n) || n == 0 || n > rtmax - rtmin)
	    return -1;
	  sig += negative ? -n : n;
	  if (sig < rtmin || sig > rtmax)
	    return -1;
	}
      *pnum = sig;
      return 0;
    }

  /* A signal name without the SIG prefix.  */
  for (int i = 1; i < array_length (__sys_sigabbrev); i++)
    if (__sys_sigabbrev[i] != NULL && strcmp (str, __sys_sigabbrev[i]) == 0)
      {
	*pnum = i;
	return 0;
      }

  return -1;
}
