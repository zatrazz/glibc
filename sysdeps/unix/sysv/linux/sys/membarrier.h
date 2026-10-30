/* Memory barriers on sets of threads.  Linux version.
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

#ifndef _SYS_MEMBARRIER_H
#define _SYS_MEMBARRIER_H	1

#include <features.h>

#ifdef __has_include
# if __has_include ("linux/membarrier.h")
#  include "linux/membarrier.h"
# endif
#endif

#ifndef _LINUX_MEMBARRIER_H
enum membarrier_cmd
{
  MEMBARRIER_CMD_QUERY = 0,
  MEMBARRIER_CMD_GLOBAL = (1 << 0),
  MEMBARRIER_CMD_GLOBAL_EXPEDITED = (1 << 1),
  MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED = (1 << 2),
  MEMBARRIER_CMD_PRIVATE_EXPEDITED = (1 << 3),
  MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED = (1 << 4),
  MEMBARRIER_CMD_PRIVATE_EXPEDITED_SYNC_CORE = (1 << 5),
  MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED_SYNC_CORE = (1 << 6),
  MEMBARRIER_CMD_PRIVATE_EXPEDITED_RSEQ = (1 << 7),
  MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED_RSEQ = (1 << 8),
  MEMBARRIER_CMD_GET_REGISTRATIONS = (1 << 9),

  MEMBARRIER_CMD_SHARED = MEMBARRIER_CMD_GLOBAL
};

enum membarrier_cmd_flag
{
  /* Restrict MEMBARRIER_CMD_PRIVATE_EXPEDITED_RSEQ to the CPU given by the
     CPU_ID argument.  */
  MEMBARRIER_CMD_FLAG_CPU = (1 << 0)
};
#endif /* !_LINUX_MEMBARRIER_H */

__BEGIN_DECLS

/* Issue the memory barrier selected by CMD on a set of threads.  FLAGS is
   zero or MEMBARRIER_CMD_FLAG_CPU, in which case CPU_ID selects the CPU
   (otherwise it is ignored).  Returns 0 (or the mask of supported commands
   for MEMBARRIER_CMD_QUERY, or the mask of registered commands for
   MEMBARRIER_CMD_GET_REGISTRATIONS) on success, or -1 and sets errno on
   failure.  */
extern int membarrier (int __cmd, unsigned int __flags, int __cpu_id)
     __THROW;

__END_DECLS

#endif /* sys/membarrier.h */
