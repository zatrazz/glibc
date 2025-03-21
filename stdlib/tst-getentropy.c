/* Tests for the getentropy function.
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

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>

#ifndef ERRNO_BUFFER_TO_LARGE
# define ERRNO_BUFFER_TO_LARGE EINVAL
#endif

/* Set to true if any errors are encountered.  */
static bool errors;

static void
test_getentropy (void)
{
  char buf[16];
  memset (buf, '@', sizeof (buf));
  if (getentropy (buf, 0) != 0)
    {
      printf ("error: getentropy zero length: %m\n");
      errors = true;
      return;
    }
  for (size_t i = 0; i < sizeof (buf); ++i)
    if (buf[i] != '@')
      {
        printf ("error: getentropy modified zero-length buffer\n");
        errors = true;
        return;
      }

  if (getentropy (buf, sizeof (buf)) != 0)
    {
      printf ("error: getentropy buf: %m\n");
      errors = true;
      return;
    }

  char buf2[256];
  _Static_assert (sizeof (buf) < sizeof (buf2), "buf and buf2 compatible");
  memset (buf2, '@', sizeof (buf2));
  if (getentropy (buf2, sizeof (buf)) != 0)
    {
      printf ("error: getentropy buf2: %m\n");
      errors = true;
      return;
    }

  /* The probability that these two buffers are equal is very
     small. */
  if (memcmp (buf, buf2, sizeof (buf)) == 0)
    {
      printf ("error: getentropy appears to return constant bytes\n");
      errors = true;
      return;
    }

  for (size_t i = sizeof (buf); i < sizeof (buf2); ++i)
    if (buf2[i] != '@')
      {
        printf ("error: getentropy wrote beyond the end of the buffer\n");
        errors = true;
        return;
      }

  char buf3[257];
  if (getentropy (buf3, sizeof (buf3)) == 0)
    {
      printf ("error: getentropy successful for 257 byte buffer\n");
      errors = true;
      return;
    }
  if (errno != ERRNO_BUFFER_TO_LARGE)
    {
      printf ("error: getentropy wrong error for 257 byte buffer: %m\n");
      errors = true;
      return;
    }
}

static int
do_test (void)
{
  test_getentropy ();

  return errors;
}

#include <support/test-driver.c>
