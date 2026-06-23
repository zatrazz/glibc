/* Generate a temporary file name and create it through a callback.
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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Generate a unique temporary name of the form <PREFIX><RANDOM><SUFFIX>,
   where RANDOM is N_RANDOM random characters (six if N_RANDOM is zero), and
   create the file by calling TRYFUNC.

   PREFIX and SUFFIX default to the empty string when NULL.  Unlike the
   mk[s]temp family, the name is neither a mutated template nor tied to a
   fixed creation policy: TRYFUNC (name, ARGS) is responsible for creating
   the file by whatever means it likes (open, openat, openat2, mkdirat,
   symlinkat, ...) and returns the resulting descriptor (or any other
   non-negative value).  If TRYFUNC returns -1 with errno set to EEXIST a
   fresh name is generated and it is called again; any other failure aborts.
   ARGS is passed to TRYFUNC verbatim and is otherwise opaque, so the
   callback signature stays free of any size-sensitive type.

   On success the value returned by TRYFUNC is returned and, when NAMEOUT is
   not NULL, *NAMEOUT is set to a newly allocated string holding the chosen
   name, which the caller must free.  On failure -1 is returned with errno
   set.  */
int
mkostempfn (const char *prefix, unsigned int n_random, const char *suffix,
	    int (*tryfunc) (char *, void *), void *args, char **nameout)
{
  if (prefix == NULL)
    prefix = "";
  if (suffix == NULL)
    suffix = "";
  if (n_random == 0)
    n_random = 6;

  size_t prefixlen = strlen (prefix);
  size_t suffixlen = strlen (suffix);

  /* Compute the buffer size (name plus the terminating NUL), guarding
     against overflow of size_t and of the int suffix length the back end
     uses.  */
  if (suffixlen > INT_MAX
      || prefixlen > SIZE_MAX - n_random
      || prefixlen + n_random > SIZE_MAX - suffixlen
      || prefixlen + n_random + suffixlen > SIZE_MAX - 1)
    {
      __set_errno (ENAMETOOLONG);
      return -1;
    }
  size_t bufsize = prefixlen + n_random + suffixlen + 1;

  char *buf = malloc (bufsize);
  if (buf == NULL)
    return -1;

  /* Assemble <prefix><n_random 'X's><suffix>; the back end overwrites the
     'X' placeholders in place with random characters on each attempt.  */
  char *p = __mempcpy (buf, prefix, prefixlen);
  memset (p, 'X', n_random);
  p += n_random;
  p = __mempcpy (p, suffix, suffixlen);
  *p = '\0';

  int fd = __try_tempname_len (buf, suffixlen, args, tryfunc, n_random);

  if (fd < 0 || nameout == NULL)
    {
      int saved_errno = errno;
      free (buf);
      __set_errno (saved_errno);
    }
  else
    *nameout = buf;

  return fd;
}
