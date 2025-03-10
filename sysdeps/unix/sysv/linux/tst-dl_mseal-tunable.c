/* Check glibc.rtld.seal tunable.
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

/* Check if memory sealing is not applied if glibc.rtld.seal is set to 0. */

#define TUNABLE_ENV_VAR (char *)"GLIBC_TUNABLES=glibc.rtld.seal=0"

#define TEST_NAME "tst-dl_mseal-tunable"
#include "tst-dl_mseal-noseal.c"
