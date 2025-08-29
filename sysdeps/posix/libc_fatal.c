/* Catastrophic failure reports.  Generic POSIX.1 version.
   Copyright (C) 1993-2025 Free Software Foundation, Inc.
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

/* Mark symbols hidden in static PIE for early self relocation to work.  */
#if BUILD_PIE_DEFAULT
# pragma GCC visibility push(hidden)
#endif
#include <dl-writev.h>
#include <dl-mmap.h>
#include <assert.h>
#include <ldsodefs.h>
#include <setvmaname.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/uio.h>
#include <unistd.h>
#include <dl-symbol-redir-ifunc.h>

#ifdef FATAL_PREPARE_INCLUDE
#include FATAL_PREPARE_INCLUDE
#endif

static void
writev_for_fatal (int fd, const struct iovec *iov, size_t niov)
{
  ssize_t cnt;
  do
    cnt = _dl_writev (fd, iov, niov);
  while (cnt == -EINTR);
}

/* At most a substring before each conversion specification and the
   trailing substring (the plus one).  */
#define IOVEC_MAX (LIBC_MESSAGE_MAX_ARGS * 2 + 1)

/* Abort with an error message.  */
void
__libc_message_impl (const char *fmt, ...)
{
  va_list ap;
  int fd = -1;

#ifdef FATAL_PREPARE
  FATAL_PREPARE;
#endif

  if (fd == -1)
    fd = STDERR_FILENO;

  struct iovec iov[IOVEC_MAX];
  int iovcnt = 0;
  ssize_t total = 0;

  va_start (ap, fmt);
  const char *cp = fmt;
  while (*cp != '\0')
    {
      /* Find the next "%s" or the end of the string.  */
      const char *next = cp;
      while (next[0] != '%' || next[1] != 's')
	{
	  next = __strchrnul (next + 1, '%');

	  if (next[0] == '\0')
	    break;
	}

      /* Determine what to print.  */
      const char *str;
      size_t len;
      if (cp[0] == '%' && cp[1] == 's')
	{
	  str = va_arg (ap, const char *);
	  len = strlen (str);
	  cp += 2;
	}
      else
	{
	  str = cp;
	  len = next - cp;
	  cp = next;
	}

      iov[iovcnt].iov_base = (char *) str;
      iov[iovcnt].iov_len = len;
      total += len;
      iovcnt++;

      if (__glibc_unlikely (iovcnt > IOVEC_MAX))
	{
	  len = IOVEC_MAX_ERR_MSG_LEN;
	  iov[0].iov_base = (char *) IOVEC_MAX_ERR_MSG;
	  iov[0].iov_len = len;
	  total = len;
	  iovcnt = 1;
	  break;
	}
    }
  va_end (ap);

  if (iovcnt > 0)
    {
      writev_for_fatal (fd, iov, iovcnt);

      total = ALIGN_UP (total + sizeof (struct abort_msg_s) + 1,
			GLRO(dl_pagesize));
      struct abort_msg_s *buf = _dl_mmap (NULL, total,
					  PROT_READ | PROT_WRITE,
					  MAP_ANON | MAP_PRIVATE);
      if (__glibc_likely (buf != MAP_FAILED))
	{
	  buf->size = total;
	  char *wp = buf->msg;
	  for (int cnt = 0; cnt < iovcnt; ++cnt)
	    wp = mempcpy (wp, iov[cnt].iov_base, iov[cnt].iov_len);
	  *wp = '\0';

	  __set_vma_name (buf, total, " glibc: fatal");

	  /* We have to free the old buffer since the application might
	     catch the SIGABRT signal.  */
	  struct abort_msg_s *old = atomic_exchange_acquire (&__abort_msg,
							     buf);
	  if (old != NULL)
	    __munmap (old, old->size);
	}
    }

  /* Kill the application.  */
  abort ();
}


void
__libc_fatal (const char *message)
{
  /* The loop is added only to keep gcc happy.  */
  while (1)
    __libc_message ("%s", message);
}
libc_hidden_def (__libc_fatal)
