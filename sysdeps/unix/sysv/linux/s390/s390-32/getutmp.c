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

#include <string.h>
#include <utmp.h>
#define getutmpx __redirect_getutmpx
#include <utmpx.h>
#undef getutmpx

#include "utmp-compat.h"

void
__getutmp (const struct utmpx *utmpx, struct utmp *utmp)
{
}
symbol_version (__getutmp, getutmp, GLIBC_2.1.1);
symbol_version (__getutmp, getutmpx, GLIBC_2.1.1);

default_symbol_version (__getutmp, getutmp, UTMP_COMPAT_BASE);
default_symbol_version (__getutmp, getutmpx, UTMP_COMPAT_BASE);
stub_warning (getutmp)
stub_warning (getutmpx)
