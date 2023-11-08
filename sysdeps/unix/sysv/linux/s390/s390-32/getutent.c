/* Copyright (C) 2008-2024 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <utmp.h>

#include "utmp-compat.h"

struct utmp *
__getutent (void)
{
  return NULL;
}
symbol_version (__getutent, getutent, GLIBC_2.0);
symbol_version (__getutent, getutxent, GLIBC_2.1);

#if defined SHARED
default_symbol_version (__getutent, getutent, UTMP_COMPAT_BASE);
default_symbol_version (__getutent, getutxent, UTMP_COMPAT_BASE);
#else
weak_alias (__getutent, getutent)
weak_alias (__getutent, getutxent)
#endif
stub_warning (getutent)
stub_warning (getutxent)
