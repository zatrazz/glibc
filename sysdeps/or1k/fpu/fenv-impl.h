/* Architecture-specific implementation of the <fenv.h> functions.
   OpenRISC version.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.
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

#ifndef OR1K_FENV_IMPL_H
#define OR1K_FENV_IMPL_H 1

#include <fenv.h>
#include <fpu_control.h>
#include <get-rounding-mode.h>
#include <float.h>
#include <math.h>

/* The inline functions used both by the fenv functions and by the libm
   internal <fenv_private.h> hooks.  */

static __always_inline void
libc_fesetround_or1k (int round)
{
  fpu_control_t cw;
  fpu_control_t cw_new;

  _FPU_GETCW (cw);
  cw_new = cw & ~_FPU_FPCSR_RM_MASK;
  cw_new |= round;
  if (cw != cw_new)
    _FPU_SETCW (cw_new);
}

static __always_inline int
libc_feupdateenv_test_or1k (const fenv_t *envp, int ex)
{
  fpu_control_t cw;
  fpu_control_t cw_new;
  int excepts;

  /* Get current control word.  */
  _FPU_GETCW (cw);

  /* Merge current exception flags with the passed fenv.  */
  excepts = cw & FE_ALL_EXCEPT;
  cw_new = (envp == FE_DFL_ENV ? _FPU_DEFAULT : *envp) | excepts;

  if (__glibc_unlikely (cw != cw_new))
    _FPU_SETCW (cw_new);

  /* Raise the exceptions if enabled in the new FP state.  */
  if (__glibc_unlikely (excepts))
    __feraiseexcept (excepts);

  return excepts & ex;
}

static __always_inline int
fenv_clearexcept (int excepts)
{
  fpu_control_t cw;
  fpu_control_t cw_new;

  /* Mask out unsupported bits/exceptions.  */
  excepts &= FE_ALL_EXCEPT;

  /* Read the complete control word.  */
  _FPU_GETCW (cw);

  cw_new = cw & ~excepts;

  /* Put the new data in effect.  */
  if (cw != cw_new)
    _FPU_SETCW (cw_new);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  fpu_control_t cw;

  /* Get current control word.  */
  _FPU_GETCW (cw);

  /* Check if any of the queried exception flags are set.  */
  *flagp = cw & excepts & FE_ALL_EXCEPT;

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  if (excepts == 0)
    return 0;

  /* Raise exceptions represented by EXPECTS.  */

  if (excepts & FE_INEXACT)
  {
    float d = 1.0, x = 3.0;
    __asm__ volatile ("lf.div.s %0, %0, %1" : "+r" (d) : "r" (x));
  }

  if (excepts & FE_UNDERFLOW)
  {
    float d = FLT_MIN;
    __asm__ volatile ("lf.mul.s %0, %0, %0" : "+r" (d));
  }

  if (excepts & FE_OVERFLOW)
  {
    float d = FLT_MAX;
    __asm__ volatile ("lf.mul.s %0, %0, %0" : "+r" (d) : "r" (d));
  }

  if (excepts & FE_DIVBYZERO)
  {
    float d = 1.0, x = 0.0;
    __asm__ volatile ("lf.div.s %0, %0, %1" : "+r" (d) : "r" (x));
  }

  if (excepts & FE_INVALID)
  {
    float d = HUGE_VAL, x = 0.0;
    __asm__ volatile ("lf.mul.s %0, %1, %0" : "+r" (d) : "r" (x));
  }

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fpu_control_t cw;
  fpu_control_t cw_new;

  _FPU_GETCW (cw);
  cw_new = cw | (excepts & FE_ALL_EXCEPT);
  if (cw != cw_new)
    _FPU_SETCW (cw_new);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fpu_control_t cw;
  fpu_control_t cw_new;

  /* Get the current exceptions.  */
  _FPU_GETCW (cw);

  /* Make sure the flags we want restored are legal.  */
  excepts &= FE_ALL_EXCEPT;

  /* Now set selected bits from flagp. Note that we ignore all non-flag
     bits from *flagp, so they don't matter.  */
  cw_new = (cw & ~excepts) | (*flagp & excepts);

  if (cw != cw_new)
    _FPU_SETCW (cw_new);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  fpu_control_t cw;

  /* Get current control word.  */
  _FPU_GETCW (cw);

  /* Check if any of the queried exception flags are set.  */
  return cw & excepts & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getround (void)
{
  return get_rounding_mode ();
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
      libc_fesetround_or1k (round);
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
  fpu_control_t cw;
  fpu_control_t cw_new;

  /* Get and store the environment.  */
  _FPU_GETCW (cw);
  *envp = cw;

  /* Clear the exception status flags.  */
  cw_new = cw & ~FE_ALL_EXCEPT;

  if (cw != cw_new)
    _FPU_SETCW (cw_new);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  if (envp == FE_DFL_ENV)
    _FPU_SETCW (_FPU_DEFAULT);
  else
    _FPU_SETCW (*envp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  libc_feupdateenv_test_or1k (envp, 0);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  _FPU_GETCW (*modep);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  fpu_control_t cw;
  fpu_control_t cw_new;

  _FPU_GETCW (cw);
  cw_new = cw & ~_FPU_FPCSR_RM_MASK;
  if (modep == FE_DFL_MODE)
    cw_new |= (_FPU_DEFAULT & _FPU_FPCSR_RM_MASK);
  else
    cw_new |= (*modep & _FPU_FPCSR_RM_MASK);
  if (cw != cw_new)
    _FPU_SETCW (cw_new);

  /* Success.  */
  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

/* OpenRISC has no support for trapping exceptions.  */

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
