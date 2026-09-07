/* Private floating point rounding and exceptions handling.  RISC-V version.
   Copyright (C) 2014-2026 Free Software Foundation, Inc.
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

#ifndef RISCV_FENV_PRIVATE_H
#define RISCV_FENV_PRIVATE_H 1

#include <fenv.h>
#include <fenv-impl.h>
#include <fpu_control.h>
#include <get-rounding-mode.h>

static __always_inline void
libc_fesetround_riscv (int round)
{
  riscv_setround (round);
}

#define libc_fesetround  libc_fesetround_riscv

static __always_inline void
libc_feholdexcept_setround_riscv (fenv_t *envp, int round)
{
  libc_feholdexcept_riscv (envp);
  libc_fesetround_riscv (round);
}

#define libc_feholdexcept_setround  libc_feholdexcept_setround_riscv

static __always_inline int
libc_feupdateenv_test_riscv (const fenv_t *envp, int ex)
{
  fenv_t env = *envp;
  int flags = riscv_getflags ();
  asm volatile ("csrw fcsr, %z0" : : "rJ" (env | flags));
  return flags & ex;
}

#define libc_feupdateenv_test  libc_feupdateenv_test_riscv

static __always_inline void
libc_feholdsetround_riscv (fenv_t *envp, int round)
{
  /* Note this implementation makes an improperly-formatted fenv_t and
     so should only be used in conjunction with libc_feresetround.  */
  int old_round;
  asm volatile ("csrrw %0, frm, %z1" : "=r" (old_round) : "rJ" (round));
  *envp = old_round;
}

#define libc_feholdsetround  libc_feholdsetround_riscv

static __always_inline void
libc_feresetround_riscv (fenv_t *envp)
{
  /* Note this implementation takes an improperly-formatted fenv_t and
     so should only be used in conjunction with libc_feholdsetround.  */
  riscv_setround (*envp);
}

#define libc_feresetround  libc_feresetround_riscv

#include_next <fenv_private.h>

#endif
