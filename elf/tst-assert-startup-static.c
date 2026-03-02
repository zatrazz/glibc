/* Check if assert works during program startup.
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

#include <stdlib.h>
#include <assert.h>

/* The __tunables_init is called just before self-relocation and TLS setup,
   and the __libc_assert_fail is used internally for assert() calls.  */
extern _Noreturn __typeof (__assert_fail) __libc_assert_fail;

void __tunables_init (char **env)
{
/* The assert called by the loader/startup issues __libc_assert_fail instead
   of __libc_assert, and __libc_assert_fail does issues the translation
   routines (which would require additional handling to be called at this
   point, like disable stack protection).  So issue the internal routine
   directly, instead of using assert here.  */
  __libc_assert_fail ("error", __FILE__, __LINE__, __func__);
}

int main (int argc, char *argv[])
{
  /* Fail with a different error code than abort.  */
  exit (EXIT_FAILURE);
}
