/* Unwind information for the powerpc32 context functions.
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

#define GPR_CFI(n) \
  cfi_offset (n, _UC_GREGS + (n) * 4)
/* DWARF numbers the FPRs from 32.  */
#define FPR_CFI(dwarf, n) \
  cfi_offset (dwarf, _UC_FREGS + (n) * 8)

/* Describe the context whose registers (uc_regs) are at BASE to the
   unwinder, with the return address in the saved link register.  */
#define CONTEXT_CFI(base) \
  cfi_def_cfa (base, 0); \
  GPR_CFI (1); \
  GPR_CFI (2); \
  GPR_CFI (14); \
  GPR_CFI (15); \
  GPR_CFI (16); \
  GPR_CFI (17); \
  GPR_CFI (18); \
  GPR_CFI (19); \
  GPR_CFI (20); \
  GPR_CFI (21); \
  GPR_CFI (22); \
  GPR_CFI (23); \
  GPR_CFI (24); \
  GPR_CFI (25); \
  GPR_CFI (26); \
  GPR_CFI (27); \
  GPR_CFI (28); \
  GPR_CFI (29); \
  GPR_CFI (30); \
  GPR_CFI (31); \
  cfi_offset (lr, _UC_GREGS + PT_LNK * 4)

#define CONTEXT_FPR_CFI \
  FPR_CFI (46, 14); \
  FPR_CFI (47, 15); \
  FPR_CFI (48, 16); \
  FPR_CFI (49, 17); \
  FPR_CFI (50, 18); \
  FPR_CFI (51, 19); \
  FPR_CFI (52, 20); \
  FPR_CFI (53, 21); \
  FPR_CFI (54, 22); \
  FPR_CFI (55, 23); \
  FPR_CFI (56, 24); \
  FPR_CFI (57, 25); \
  FPR_CFI (58, 26); \
  FPR_CFI (59, 27); \
  FPR_CFI (60, 28); \
  FPR_CFI (61, 29); \
  FPR_CFI (62, 30); \
  FPR_CFI (63, 31)
