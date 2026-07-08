/* Module for tst-origin-secure: the attacker-controlled copy, reachable
   only by resolving the "sub" symlink in the un-normalized rpath.  It
   shares the soname of tst-origin-secure-mod.so (set in the Makefile).
   See tst-origin-secure.c.
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

/* 2 marks the untrusted copy: if the victim returns this, the loader
   opened the un-normalized rpath and resolved it through the attacker's
   symlink -- i.e. the trusted-path check was bypassed (bug 34360).  */
int
origin_secure_id (void)
{
  return 2;
}
