/* Read directory entries as struct posix_dent records.  Linux version.
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

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <sysdep.h>

#if defined __USE_FILE_OFFSET64 || defined __INO_T_MATCHES_INO64_T
/* struct posix_dent must match the layout of the kernel
   linux_dirent64, so that the getdents64 syscall can fill in the
   buffer directly.  */
_Static_assert (offsetof (struct posix_dent, d_ino) == 0,
		"d_ino layout mismatch");
_Static_assert (offsetof (struct posix_dent, d_off) == 8,
		"d_off layout mismatch");
_Static_assert (offsetof (struct posix_dent, d_reclen) == 16,
		"d_reclen layout mismatch");
_Static_assert (offsetof (struct posix_dent, d_type) == 18,
		"d_type layout mismatch");
_Static_assert (offsetof (struct posix_dent, d_name) == 19,
		"d_name layout mismatch");
#endif

ssize_t
posix_getdents (int fd, void *buf, size_t nbytes, int flags)
{
  /* No flags are currently defined.  */
  if (flags != 0)
    {
      __set_errno (EINVAL);
      return -1;
    }

  /* The kernel counts the buffer size in an unsigned int.  */
  if (nbytes > INT_MAX)
    nbytes = INT_MAX;

  return INLINE_SYSCALL_CALL (getdents64, fd, buf, nbytes);
}
