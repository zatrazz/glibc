/* I/O scheduling priority.  Linux version.
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

#ifndef _SYS_IOPRIO_H
#define _SYS_IOPRIO_H	1

#include <features.h>

#ifdef __has_include
# if __has_include ("linux/ioprio.h")
#  include "linux/ioprio.h"
# endif
#endif

#ifndef IOPRIO_CLASS_SHIFT
/* An I/O priority value is made of a class and of a level (whose meaning
   depends on the class).  */
# define IOPRIO_CLASS_SHIFT	13
# define IOPRIO_CLASS_MASK	0x07
# define IOPRIO_PRIO_MASK	((1UL << IOPRIO_CLASS_SHIFT) - 1)

# define IOPRIO_PRIO_CLASS(ioprio) \
  (((ioprio) >> IOPRIO_CLASS_SHIFT) & IOPRIO_CLASS_MASK)
# define IOPRIO_PRIO_DATA(ioprio) ((ioprio) & IOPRIO_PRIO_MASK)
# define IOPRIO_PRIO_VALUE(prioclass, priolevel) \
  ((((prioclass) & IOPRIO_CLASS_MASK) << IOPRIO_CLASS_SHIFT) \
   | ((priolevel) & IOPRIO_PRIO_MASK))

/* I/O priority classes.  */
enum
{
  IOPRIO_CLASS_NONE,
  IOPRIO_CLASS_RT,
  IOPRIO_CLASS_BE,
  IOPRIO_CLASS_IDLE,
};

/* Number of levels of the real-time and best-effort classes.  */
# define IOPRIO_NR_LEVELS	8
# define IOPRIO_BE_NR		IOPRIO_NR_LEVELS

/* The WHICH argument of ioprio_get and ioprio_set.  */
enum
{
  IOPRIO_WHO_PROCESS = 1,
  IOPRIO_WHO_PGRP,
  IOPRIO_WHO_USER,
};

/* Default level of the best-effort class.  */
# define IOPRIO_NORM	4
# define IOPRIO_BE_NORM	IOPRIO_NORM
#endif /* !IOPRIO_CLASS_SHIFT */

__BEGIN_DECLS

/* Return the I/O priority of the process, process group, or user (as selected
   by WHICH, one of the IOPRIO_WHO_* values) identified by WHO.  If WHO is
   zero, the calling process, its process group, or its real user ID is used.
   Return -1 and set errno on failure.  */
extern int ioprio_get (int __which, int __who) __THROW;

/* Set the I/O priority of the process, process group, or user (as selected by
   WHICH) identified by WHO to IOPRIO, which is usually built with
   IOPRIO_PRIO_VALUE.  Return 0 on success, -1 and set errno on failure.  */
extern int ioprio_set (int __which, int __who, int __ioprio) __THROW;

__END_DECLS

#endif /* sys/ioprio.h */
