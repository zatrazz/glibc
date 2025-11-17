/* Module for tst-thrlocal-dlclose test.
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

static int *fini_flag;

extern "C" void
set_fini_flag (int *p)
{
  fini_flag = p;
}

static struct lib2_dtor
{
  ~lib2_dtor ()
  {
    if (fini_flag != nullptr)
      *fini_flag = 1;
  }
} lib2_dtor_obj;

int foo (void) { return 42; }
