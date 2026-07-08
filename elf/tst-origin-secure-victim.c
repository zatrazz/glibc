/* Victim program for tst-origin-secure.  It is linked against
   libtst-origin-secure-mod.so with the rpath

     $ORIGIN/sub/../../../../..SLIBDIR/tst-origin-secure

   (see the Makefile).  Lexically that normalizes to the trusted
   SLIBDIR/tst-origin-secure, but if the loader opens the un-normalized
   string it resolves "sub" as a symlink first and lands in an
   attacker-controlled directory.  See tst-origin-secure.c.
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

#include <sys/auxv.h>

extern int origin_secure_id (void);

/* Report both which module was loaded and whether the loader placed the
   program in AT_SECURE mode, so the driver can tell a genuine trusted-path
   bypass apart from a run that simply was not secure.  Bit 0 is set when
   the attacker copy (id 2) was loaded; bit 1 is set when AT_SECURE was in
   effect.  The exit status is therefore one of:

     0  trusted copy, not secure   (standalone smoke run: harmless)
     1  attacker copy, not secure  (the control run)
     2  trusted copy, AT_SECURE    (a fixed loader)
     3  attacker copy, AT_SECURE   (the bug: the raw rpath was opened)

   Exiting 0 in the standalone case keeps this program harmless when the
   test harness builds and runs it directly (it then finds the trusted
   module via the normal search path and is not in secure mode).  */
int
main (void)
{
  int loaded_attacker = origin_secure_id () == 2;
  int at_secure = getauxval (AT_SECURE) != 0;
  return loaded_attacker | (at_secure << 1);
}
