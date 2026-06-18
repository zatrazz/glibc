/* Generate a temporary file relative to a directory descriptor.
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
#include <fcntl.h>
#include <intprops.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scratch_buffer.h>

static int
mkostempat_create (int dirfd, char *buf, const char *prefix, size_t prefixlen,
		   unsigned int n_random, const char *suffix, size_t suffixlen,
		   int oflags, mode_t mode)
{
  char *p = __mempcpy (buf, prefix, prefixlen);
  memset (p, 'X', n_random);
  p += n_random;
  p = __mempcpy (p, suffix, suffixlen);
  *p = '\0';

  return __gen_tempname_at (dirfd, buf, suffixlen,
			    oflags | O_CLOEXEC | O_LARGEFILE, mode, n_random);
}

/* Used when the caller does not want the generated name back: assemble it
   in a scratch buffer (on the stack for short names) and create the file.
   The scratch buffer lives in this separate non-inlined frame so that
   callers which do want the name (and never reach here) do not pay its
   on-stack cost.  */
static int __attribute_noinline__
mkostempat_scratch (int dirfd, size_t bufsize, const char *prefix,
		    size_t prefixlen, unsigned int n_random,
		    const char *suffix, size_t suffixlen, int oflags,
		    mode_t mode)
{
  struct scratch_buffer sbuf;
  scratch_buffer_init (&sbuf);
  if (!scratch_buffer_set_array_size (&sbuf, bufsize, 1))
    return -1;
  int fd = mkostempat_create (dirfd, sbuf.data, prefix, prefixlen, n_random,
			      suffix, suffixlen, oflags, mode);
  int saved_errno = errno;
  scratch_buffer_free (&sbuf);
  __set_errno (saved_errno);
  return fd;
}

int
mkostempat (int dirfd, const char *prefix, const char *suffix,
	    unsigned int n_random, int oflags, mode_t mode, char **namebuf)
{
  /* A null PREFIX or SUFFIX defaults to the empty string.  Both must be a
     single path component: rejecting '/' keeps the generated name a leaf
     created relative to DIRFD, so the operation never walks an
     attacker-controllable path and stays free of the TOCTOU races that
     motivate a directory-descriptor interface.  */
  if (prefix == NULL)
    prefix = "";
  if (suffix == NULL)
    suffix = "";

  if (strchr (prefix, '/') != NULL || strchr (suffix, '/') != NULL
      || (n_random != 0 && n_random < GEN_TEMPNAME_MIN_SUFFIX_LEN))
    {
      __set_errno (EINVAL);
      return -1;
    }

#ifdef O_TMPFILE
  /* O_TMPFILE creates an unnamed file in a directory and is fundamentally
     incompatible with this interface, which generates a name and creates it
     with O_CREAT | O_EXCL.  An unnamed file cannot be atomically installed
     under a name anyway (there is no flink and linkat cannot replace), so
     reject the flag rather than relying on the kernel to fault the
     O_TMPFILE | O_CREAT combination; the caller can use the named result
     and renameat for atomic replacement instead.  */
  if ((oflags & O_TMPFILE) == O_TMPFILE)
    {
      __set_errno (EINVAL);
      return -1;
    }
#endif

  if (n_random == 0)
    n_random = GEN_TEMPNAME_DEF_SUFFIX_LEN;

  size_t prefixlen = strlen (prefix);
  size_t suffixlen = strlen (suffix);

  size_t bufsize;
  if (suffixlen > INT_MAX
      || INT_ADD_WRAPV (prefixlen, n_random, &bufsize)
      || INT_ADD_WRAPV (bufsize, suffixlen, &bufsize)
      || INT_ADD_WRAPV (bufsize, 1, &bufsize)) /* '\0' */
    {
      __set_errno (ENAMETOOLONG);
      return -1;
    }

  /* When the caller does not want the name, build it in a scratch buffer.  */
  if (namebuf == NULL)
    return mkostempat_scratch (dirfd, bufsize, prefix, prefixlen, n_random,
			       suffix, suffixlen, oflags, mode);

  /* Otherwise hand back a heap-allocated name that the caller must free.  */
  char *buf = malloc (bufsize);
  if (buf == NULL)
    return -1;

  int fd = mkostempat_create (dirfd, buf, prefix, prefixlen, n_random,
			      suffix, suffixlen, oflags, mode);
  if (fd < 0)
    {
      int saved_errno = errno;
      free (buf);
      __set_errno (saved_errno);
      return -1;
    }

  *namebuf = buf;
  return fd;
}
