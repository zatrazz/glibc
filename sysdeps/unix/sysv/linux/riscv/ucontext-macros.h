/* Macros for ucontext routines.
   Copyright (C) 2017-2026 Free Software Foundation, Inc.
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
   License along with the GNU C Library.  If not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef _LINUX_RISCV_UCONTEXT_MACROS_H
#define _LINUX_RISCV_UCONTEXT_MACROS_H

#include <sysdep.h>
#include <sys/asm.h>

#include "ucontext_i.h"

#define MCONTEXT_FSR (32 * SZFREG + MCONTEXT_FPREGS)

#define SAVE_FP_REG(name, num, base)			\
  FREG_S name, ((num) * SZFREG + MCONTEXT_FPREGS)(base)

#define RESTORE_FP_REG(name, num, base)			\
  FREG_L name, ((num) * SZFREG + MCONTEXT_FPREGS)(base)

#define RESTORE_FP_REG_CFI(name, num, base)		\
  RESTORE_FP_REG (name, num, base);			\
  cfi_offset (name, (num) * SZFREG + MCONTEXT_FPREGS)

#define SAVE_INT_REG(name, num, base)			\
  REG_S name, ((num) * SZREG + MCONTEXT_GREGS)(base)

#define RESTORE_INT_REG(name, num, base)		\
  REG_L name, ((num) * SZREG + MCONTEXT_GREGS)(base)

#define RESTORE_INT_REG_CFI(name, num, base)		\
  RESTORE_INT_REG (name, num, base);			\
  cfi_offset (name, (num) * SZREG + MCONTEXT_GREGS)

#define INT_REG_CFI(name, num)				\
  cfi_offset (name, (num) * SZREG + MCONTEXT_GREGS)
#define FP_REG_CFI(name, num)				\
  cfi_offset (name, (num) * SZFREG + MCONTEXT_FPREGS)

/* Describe the context whose address is in BASE to the unwinder.  */
#ifndef __riscv_float_abi_soft
# define CONTEXT_FP_CFI						\
  FP_REG_CFI (fs0, 8); FP_REG_CFI (fs1, 9); FP_REG_CFI (fs2, 18);	\
  FP_REG_CFI (fs3, 19); FP_REG_CFI (fs4, 20); FP_REG_CFI (fs5, 21);	\
  FP_REG_CFI (fs6, 22); FP_REG_CFI (fs7, 23); FP_REG_CFI (fs8, 24);	\
  FP_REG_CFI (fs9, 25); FP_REG_CFI (fs10, 26); FP_REG_CFI (fs11, 27)
#else
# define CONTEXT_FP_CFI
#endif
#define CONTEXT_CFI(base)						\
  cfi_def_cfa (base, 0);						\
  INT_REG_CFI (ra, 1); INT_REG_CFI (sp, 2); INT_REG_CFI (s0, 8);	\
  INT_REG_CFI (s1, 9); INT_REG_CFI (s2, 18); INT_REG_CFI (s3, 19);	\
  INT_REG_CFI (s4, 20); INT_REG_CFI (s5, 21); INT_REG_CFI (s6, 22);	\
  INT_REG_CFI (s7, 23); INT_REG_CFI (s8, 24); INT_REG_CFI (s9, 25);	\
  INT_REG_CFI (s10, 26); INT_REG_CFI (s11, 27);			\
  CONTEXT_FP_CFI

#endif /* _LINUX_RISCV_UCONTEXT_MACROS_H */
