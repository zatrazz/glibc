/* Stub for getentropy.
   Copyright (C) 2016-2025 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <errno.h>
#include <not-cancel.h>
#include <sys/random.h>
#include <shlib-compat.h>

/* Write LENGTH bytes of randomness starting at BUFFER.  Return 0 on
   success and -1 on failure.  */
static int
getentropy_base (void *buffer, size_t length, int err)
{
  if (length > 256)
    {
      __set_errno (err);
      return -1;
    }

  /* Try to fill the buffer completely.  Even with the 256 byte limit
     above, we might still receive an EINTR error (when blocking
     during boot).  */
  void *end = buffer + length;
  while (buffer < end)
    {
      /* NB: No cancellation point.  */
      ssize_t bytes = __getrandom_nocancel (buffer, end - buffer, 0);
      if (bytes < 0)
        {
          if (errno == EINTR)
            /* Try again if interrupted by a signal.  */
            continue;
          else
            return -1;
        }
      else if (bytes == 0)
        /* No more bytes available.  This should not happen under normal
	   circumstances.  */
	{
	  __set_errno (err);
	  return -1;
	}

      /* Try again in case of a short read.  */
      buffer += bytes;
    }
  return 0;
}

int
__new_getentropy (void *buffer, size_t length)
{
  return getentropy_base (buffer, length, EINVAL);
}
versioned_symbol (libc, __new_getentropy, getentropy, GLIBC_2_42);

#if SHLIB_COMPAT (libc, GLIBC_2_25, GLIBC_2_42)
int
__old_getentropy (void *buffer, size_t length)
{
  return getentropy_base (buffer, length, EIO);
}
compat_symbol (libc, __old_getentropy, getentropy, GLIBC_2_25);
#endif
