/* Close a file descriptor, with well-defined interruption semantics.
   Linux version.
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
#include <sysdep-cancel.h>
#include <unistd.h>

int
posix_close (int fd, int flags)
{
  /* Linux always releases the file descriptor even when the close
     system call fails, including when it is interrupted by a signal,
     so POSIX_CLOSE_RESTART is defined to 0 and FLAGS is ignored, as
     on musl.  */
  int r = SYSCALL_CANCEL (close, fd);
  if (r < 0 && errno == EINTR)
    {
      /* EINTR must not be reported: it would tell the caller that the
	 operation can be retried, while retrying could close an
	 unrelated file descriptor.  Report the descriptor as closed
	 with the operation still in progress instead, as POSIX.1-2024
	 specifies.  */
      __set_errno (EINPROGRESS);
    }
  return r;
}
