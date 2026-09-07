/* Architecture-specific implementation of the <fenv.h> functions.
   RISC-V version.
   Copyright (C) 1998-2026 Free Software Foundation, Inc.
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

#ifndef RISCV_FENV_IMPL_H
#define RISCV_FENV_IMPL_H 1

#include <fenv.h>
#include <fpu_control.h>
#include <get-rounding-mode.h>

/* The inline functions used both by the fenv functions and by the libm
   internal <fenv_private.h> hooks.  */

static __always_inline int
riscv_getround (void)
{
  return get_rounding_mode ();
}

static __always_inline void
riscv_setround (int rm)
{
  asm volatile ("fsrm %z0" : : "rJ" (rm));
}

static __always_inline int
riscv_getflags (void)
{
  int flags;
  asm volatile ("frflags %0" : "=r" (flags));
  return flags;
}

static __always_inline void
riscv_setflags (int flags)
{
  asm volatile ("fsflags %z0" : : "rJ" (flags));
}

static __always_inline void
libc_feholdexcept_riscv (fenv_t *envp)
{
  asm volatile ("csrrc %0, fcsr, %1" : "=r" (*envp) : "i" (FE_ALL_EXCEPT));
}

static __always_inline int
fenv_clearexcept (int excepts)
{
  asm volatile ("csrc fflags, %0" : : "r" (excepts));
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  /* Get the current exceptions.  */
  *flagp = riscv_getflags () & excepts;

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  asm volatile ("csrs fflags, %0" : : "r" (excepts));
  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  asm volatile ("csrs fflags, %0" : : "r" (excepts));
  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fexcept_t flags = *flagp;
  asm volatile ("csrc fflags, %0" : : "r" (excepts));
  asm volatile ("csrs fflags, %0" : : "r" (flags & excepts));

  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  return riscv_getflags () & excepts;
}

static __always_inline int
fenv_getround (void)
{
  return riscv_getround ();
}

static __always_inline int
fenv_setround (int round)
{
  switch (round)
    {
    case FE_TONEAREST:
    case FE_TOWARDZERO:
    case FE_DOWNWARD:
    case FE_UPWARD:
      riscv_setround (round);
      return 0;
    default:
      return round; /* A nonzero value.  */
    }
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  _FPU_GETCW (*envp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  libc_feholdexcept_riscv (envp);
  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  long int env = (envp != FE_DFL_ENV ? *envp : 0);
  _FPU_SETCW (env);
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  long int env = (envp != FE_DFL_ENV ? *envp : 0);
  _FPU_SETCW (env | riscv_getflags ());
  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  _FPU_GETCW (*modep);
  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  asm volatile ("csrc fcsr, %0" : : "r" (~FE_ALL_EXCEPT));
  if (modep != FE_DFL_MODE)
    asm volatile ("csrs fcsr, %0" : : "r" (*modep & ~FE_ALL_EXCEPT));
  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

/* RISC-V has no support for trapping exceptions.  */

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
