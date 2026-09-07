/* Architecture-specific implementation of the <fenv.h> functions.
   SPARC version.
   Copyright (C) 1997-2026 Free Software Foundation, Inc.
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

#ifndef SPARC_FENV_IMPL_H
#define SPARC_FENV_IMPL_H 1

#include <fenv.h>
#include <fpu_control.h>
#include <float.h>
#include <math.h>
#include <math-barriers.h>

/* For internal use only: access the fp state register.  */
#define __fenv_stfsr(X)   _FPU_GETCW (X)
#define __fenv_ldfsr(X)   _FPU_SETCW (X)

/* The inline functions used both by the fenv functions and by the libm
   internal <fenv_private.h> hooks.  */

static __always_inline void
libc_fesetround (int r)
{
  fenv_t etmp;
  __fenv_stfsr(etmp);
  etmp = (etmp & ~__FE_ROUND_MASK) | (r);
  __fenv_ldfsr(etmp);
}

/* The control bits of the FSR: the rounding direction, the trap enable
   mask, and the nonstandard mode.  */
#define FPU_CONTROL_BITS 0xcfc00000UL

static __always_inline int
fenv_clearexcept (int excepts)
{
  fenv_t tmp;

  __fenv_stfsr (tmp);

  tmp &= ~(excepts & FE_ALL_EXCEPT);

  __fenv_ldfsr (tmp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  fexcept_t tmp;

  /* Get the current exceptions.  */
  __fenv_stfsr (tmp);

  *flagp = tmp & excepts & FE_ALL_EXCEPT;

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  static const struct {
    double zero, one, max, min, pi;
  } c = {
    0.0, 1.0, DBL_MAX, DBL_MIN, M_PI
  };
  double d;

  /* Raise exceptions represented by EXPECTS.  But we must raise only
     one signal at a time.  It is important the if the overflow/underflow
     exception and the inexact exception are given at the same time,
     the overflow/underflow exception follows the inexact exception.  */

  /* First: invalid exception.  */
  if ((FE_INVALID & excepts) != 0)
    {
      /* One example of an invalid operation is 0/0.  */
      __asm ("" : "=e" (d) : "0" (c.zero));
      d /= c.zero;
      math_force_eval (d);
    }

  /* Next: division by zero.  */
  if ((FE_DIVBYZERO & excepts) != 0)
    {
      __asm ("" : "=e" (d) : "0" (c.one));
      d /= c.zero;
      math_force_eval (d);
    }

  /* Next: overflow.  */
  if ((FE_OVERFLOW & excepts) != 0)
    {
      __asm ("" : "=e" (d) : "0" (c.max));
      d *= d;
      math_force_eval (d);
    }

  /* Next: underflow.  */
  if ((FE_UNDERFLOW & excepts) != 0)
    {
      __asm ("" : "=e" (d) : "0" (c.min));
      d *= d;
      math_force_eval (d);
    }

  /* Last: inexact.  */
  if ((FE_INEXACT & excepts) != 0)
    {
      __asm ("" : "=e" (d) : "0" (c.one));
      d /= c.pi;
      math_force_eval (d);
    }

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fenv_t tmp;

  __fenv_stfsr (tmp);
  tmp |= excepts & FE_ALL_EXCEPT;
  __fenv_ldfsr (tmp);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fenv_t tmp;

  __fenv_stfsr (tmp);

  tmp &= ~(excepts & FE_ALL_EXCEPT);
  tmp |= *flagp & excepts & FE_ALL_EXCEPT;

  __fenv_ldfsr (tmp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  fenv_t tmp;

  __fenv_stfsr (tmp);

  return tmp & excepts & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getround (void)
{
  fenv_t tmp;

  __fenv_stfsr (tmp);

  return tmp & __FE_ROUND_MASK;
}

static __always_inline int
fenv_setround (int round)
{
  if ((round & ~__FE_ROUND_MASK) != 0)
    /* ROUND is no valid rounding mode.  */
    return 1;

  libc_fesetround (round);

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  __fenv_stfsr (*envp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  fenv_t tmp;

  __fenv_stfsr (*envp);

  /* Set all exceptions to non-stop and clear all exceptions.  */
  tmp = *envp & ~((0x1f << 23) | FE_ALL_EXCEPT);

  __fenv_ldfsr (tmp);

  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  fenv_t dummy;

  /* Put these constants in memory explicitly, so as to cope with a
     -fPIC bug as of gcc 970624.  Making them automatic is quicker
     than loading up the pic register in this instance.  */

  if (envp == FE_DFL_ENV)
    {
      dummy = 0;
      envp = &dummy;
    }
  else if (envp == FE_NOMASK_ENV)
    {
      dummy = 0x1f << 23;
      envp = &dummy;
    }

  __fenv_ldfsr (*envp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  fexcept_t tmp;

  /* Save current exceptions.  */
  __fenv_stfsr (tmp);
  tmp &= FE_ALL_EXCEPT;

  /* Install new environment.  */
  fenv_setenv (envp);

  /* Raise the saved exception.  Incidentally for us the implementation
     defined format of the values in objects of type fexcept_t is the
     same as the ones specified using the FE_* constants.  */
  __feraiseexcept ((int) tmp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  __fenv_stfsr (*modep);
  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  femode_t fsr;

  __fenv_stfsr (fsr);
  fsr &= ~FPU_CONTROL_BITS;
  if (modep == FE_DFL_MODE)
    fsr |= _FPU_DEFAULT;
  else
    fsr |= *modep & FPU_CONTROL_BITS;
  __fenv_ldfsr (fsr);

  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  fenv_t new_exc, old_exc;

  __fenv_stfsr (new_exc);

  old_exc = (new_exc >> 18) & FE_ALL_EXCEPT;
  new_exc &= ~(((fenv_t)excepts & FE_ALL_EXCEPT) << 18);

  __fenv_ldfsr (new_exc);

  return old_exc;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  fenv_t new_exc, old_exc;

  __fenv_stfsr (new_exc);

  old_exc = (new_exc >> 18) & FE_ALL_EXCEPT;
  new_exc |= (((fenv_t)excepts & FE_ALL_EXCEPT) << 18);

  __fenv_ldfsr (new_exc);

  return old_exc;
}

static __always_inline int
fenv_getexcept (void)
{
  fenv_t exc;
  __fenv_stfsr (exc);

  return (exc >> 18) & FE_ALL_EXCEPT;
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
