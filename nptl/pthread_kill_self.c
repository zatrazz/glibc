/* Internal function to send a signal to itself.
   Copyright (C) 2025 Free Software Foundation, Inc.
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

#include <sysdep.h>
#include <pthread.h>
#include <unistd.h>

int
__pthread_kill_self (int signo)
{
  /* Use the actual TID from the kernel, so that it refers to the
     current thread even if called after vfork.  There is no
     signal blocking in this case, so that the signal is delivered
     immediately, before __pthread_kill_internal returns: a signal
     sent to the thread itself needs to be delivered
     synchronously.  (It is unclear if Linux guarantees the
     delivery of all pending signals after unblocking in the code
     below.  POSIX only guarantees delivery of a single signal,
     which may not be the right one.)  */
  pid_t tid = INTERNAL_SYSCALL_CALL (gettid);
  pid_t pid = INTERNAL_SYSCALL_CALL (getpid);
  int ret = INTERNAL_SYSCALL_CALL (tgkill, pid, tid, signo);
  return INTERNAL_SYSCALL_ERROR_P (ret) ? INTERNAL_SYSCALL_ERRNO (ret) : 0;
}
