/* Shared object with an unresolvable lazy binding.
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

/* Not defined anywhere.  The module can only be loaded with lazy
   binding, since the call below is never made.  */
extern void tst_rtld_bind_now_undefined (void);

void
tst_rtld_bind_now_call (void)
{
  tst_rtld_bind_now_undefined ();
}

int
tst_rtld_bind_now_value (void)
{
  return 42;
}
