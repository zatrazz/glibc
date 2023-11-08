/* Copyright (C) 1996-2024 Free Software Foundation, Inc.
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

#include <libc-lock.h>
#include <stdlib.h>
#include <utmp.h>


/* We need to protect the opening of the file.  */
__libc_lock_define_initialized (, __libc_utmp_lock attribute_hidden)


void
__setutent (void)
{
}
weak_alias (__setutent, setutent)
weak_alias (__setutent, setutxent)
stub_warning (setutent)
stub_warning (setutxent)


int
__getutent_r (struct utmp *buffer, struct utmp **result)
{
  errno = ENOTSUP;
  return -1;
}
weak_alias (__getutent_r, getutent_r)
stub_warning (getutent_r)


struct utmp *
__pututline (const struct utmp *data)
{
  return NULL;
}
weak_alias (__pututline, pututline)
weak_alias (__pututline, pututxline)
stub_warning (pututline)
stub_warning (pututxline)


void
__endutent (void)
{
}
weak_alias (__endutent, endutent)
weak_alias (__endutent, endutxent)
stub_warning (endutent)
