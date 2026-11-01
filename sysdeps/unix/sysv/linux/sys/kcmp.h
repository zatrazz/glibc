/* Compare kernel resources of processes.  Linux version.
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

#ifndef _SYS_KCMP_H
#define _SYS_KCMP_H	1

#include <features.h>
#include <bits/types.h>

#ifdef __has_include
# if __has_include ("linux/kcmp.h")
#  include "linux/kcmp.h"
# endif
#endif

#ifndef _LINUX_KCMP_H
/* Types of kernel resources compared by kcmp.  */
enum kcmp_type
{
  KCMP_FILE,
  KCMP_VM,
  KCMP_FILES,
  KCMP_FS,
  KCMP_SIGHAND,
  KCMP_IO,
  KCMP_SYSVSEM,
  KCMP_EPOLL_TFD,

  KCMP_TYPES
};

/* Target file of an epoll set, for KCMP_EPOLL_TFD.  */
struct kcmp_epoll_slot
{
  __uint32_t efd;	/* Epoll file descriptor.  */
  __uint32_t tfd;	/* Target file descriptor number.  */
  __uint32_t toff;	/* Target offset within the same numbered
			   sequence.  */
};
#endif /* !_LINUX_KCMP_H */

__BEGIN_DECLS

/* Compare the kernel resource of type TYPE (one of the KCMP_* values) used by
   the process PID1 with the one used by the process PID2.  For KCMP_FILE,
   IDX1 and IDX2 are file descriptors of the processes; for KCMP_EPOLL_TFD,
   IDX1 is a file descriptor of PID1 and IDX2 points to a
   struct kcmp_epoll_slot describing a target file registered in an epoll
   set of PID2.
   Return 0 if the resources are the same, 1 or 2 if the first one is
   respectively less or greater than the second one (in an arbitrary
   ordering), or -1 and set errno on failure.  */
extern int kcmp (__pid_t __pid1, __pid_t __pid2, int __type,
		 unsigned long int __idx1, unsigned long int __idx2) __THROW;

__END_DECLS

#endif /* sys/kcmp.h */
