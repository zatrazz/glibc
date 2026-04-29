/* Internal implementation of __spawn_process*.  Linux implementation.
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

#include <spawn.h>
#include <clone_internal.h>
#include <not-errno.h>

#pragma GCC optimize ("O0")

int
__spawn_process_create (process_create_id_t *procid,
			const char *path,
			const posix_spawn_file_actions_t *facts,
			const posix_spawnattr_t *attr,
			char *const argv[],
			char *const envp[])
{
  int r;

  if (__clone_pidfd_supported ())
    {
      int pidfd;
      r = __pidfd_spawn (&pidfd, path, facts, attr, argv, envp);
      if (r == 0)
	{
	  /* Both pidfd and pid_t do not allow negative values to describe a
	     new process, so use the MSB to set the identifier is for
	     pidfd.  */
	  *procid = pidfd | 0x80000000;
	  return 0;
	}

      /* Fallback to posix_spawn if file descriptor limits is reached.  */
      if (r != EMFILE && r != ENFILE)
	return r;
    }

  pid_t pid;
  r = __posix_spawn (&pid, path, facts, attr, argv, envp);
  if (r != 0)
    return r;
  *procid = pid;
  return 0;
}

int
__spawn_process_kill (process_create_id_t procid, int signo)
{
  return procid & 0x80000000
    ? __pidfd_send_signal_noerrno (procid & INT_MAX, signo, NULL, 0)
    : __kill_noerrno (procid, signo);
}

process_create_id_t
__spawn_process_wait (process_create_id_t procid, int *wstatus, int options)
{
  bool use_pidfd = procid & 0x80000000;

  siginfo_t info = { 0 };
  int waitid_opts = WEXITED;
  if (options & WNOHANG)
    waitid_opts |= WNOHANG;
  if (options & WUNTRACED)
    waitid_opts |= WSTOPPED;
  if (options & WCONTINUED)
    waitid_opts |= WCONTINUED;

  if (__waitid (use_pidfd ? P_PIDFD : P_PID,
		use_pidfd ? procid & INT_MAX : procid,
		&info,
		waitid_opts) == -1)
    return -1;

  /* Handle successful WNOHANG but without a child state change.  */
  if (info.si_pid == 0)
    return 0;

  if (wstatus != NULL)
    {
      int status = 0;
      switch (info.si_code)
	{
	case CLD_EXITED:
	  status = (info.si_status & 0xff) << 8;
	  break;
	case CLD_KILLED:
	  status = info.si_status & 0x7f;
	  break;
	case CLD_DUMPED:
	  status = (info.si_status & 0x7f) | __WCOREFLAG;
	  break;
	case CLD_STOPPED:
	case CLD_TRAPPED:
	  status = ((info.si_status & 0xff) << 8) | 0x7f;
	  break;
	case CLD_CONTINUED:
	  status = 0xffff;
	  break;
        }
      *wstatus = status;
    }

  /* With P_PIDFD, waitid populates info.si_pid with the actual Process ID of
     the child.  */
  return use_pidfd ? procid : info.si_pid;
}
