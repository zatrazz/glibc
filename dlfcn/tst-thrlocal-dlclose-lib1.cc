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

#include <memory>
#include <support/xdlfcn.h>

thread_local std::unique_ptr<int> tlp;

static struct c1
{
  void *h;

  c1 ()
  {
    h = xdlopen ("tst-thrlocal-dlclose-lib2.so", RTLD_NOW);
    tlp = std::make_unique<int>();
  }

  ~c1 ()
  {
    xdlclose (h);
  }
} c1;
