/* One of N indistinguishable modules for bench-dl-lookup.
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

/* MODNUM is set per-module by the Makefile rule.  Each module gets a
   small fixed number of public symbols, all with unique names — so
   the dynamic linker has to populate a meaningful hash table.  */

#ifndef MODNUM
# error "MODNUM not defined; this file is built per-module by elf/Makefile"
#endif

/* Three-argument concatenation, with the inner macro to force
   argument expansion before pasting.  Two-argument CAT (commonly
   defined as a##b) does not work here because the trailing _N is a
   separate token outside the macro call.  */
#define _CAT3(a, b, c) a##b##c
#define CAT3(a, b, c) _CAT3 (a, b, c)
#define FN(n) CAT3 (bench_dl_mod_fn, MODNUM, n)

/* Eight symbols per module — keeps total symbol count manageable but
   high enough that hash collisions are non-trivial.  */
int FN (_0) (int x) { return x + 0; }
int FN (_1) (int x) { return x + 1; }
int FN (_2) (int x) { return x + 2; }
int FN (_3) (int x) { return x + 3; }
int FN (_4) (int x) { return x + 4; }
int FN (_5) (int x) { return x + 5; }
int FN (_6) (int x) { return x + 6; }
int FN (_7) (int x) { return x + 7; }
