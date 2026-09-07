/* Architecture-specific implementation of the <fenv.h> functions.
   SH4 version.
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

#ifndef SH4_FENV_IMPL_H
#define SH4_FENV_IMPL_H 1

#include <fenv.h>
#include <fpu_control.h>
#include <float.h>
#include <math.h>

static __always_inline int
fenv_clearexcept (int excepts)
{
  fpu_control_t cw;

  /* Mask out unsupported bits/exceptions.  */
  excepts &= FE_ALL_EXCEPT;

  /* Read the complete control word.  */
  _FPU_GETCW (cw);

  /* Clear exception bits.  */
  cw &= ~excepts;

  /* Put the new data in effect.  */
  _FPU_SETCW (cw);

  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  fpu_control_t temp;

  /* Get the current exceptions.  */
  _FPU_GETCW (temp);

  /* We only save the relevant bits here. In particular, care has to be
     taken with the CAUSE bits, as an inadvertent restore later on could
     generate unexpected exceptions.  */

  *flagp = temp & excepts & FE_ALL_EXCEPT;

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
    double d = 1.0, x = 3.0;
    __asm__ __volatile__ ("fdiv %1, %0" : "+d" (d) : "d" (x));
  }

  if (excepts & FE_UNDERFLOW)
  {
    long double d = LDBL_MIN, x = 10;
    __asm__ __volatile__ ("fdiv %1, %0" : "+d" (d) : "d" (x));
  }

  if (excepts & FE_OVERFLOW)
  {
    long double d = LDBL_MAX;
    __asm__ __volatile__ ("fmul %0, %0" : "+d" (d) : "d" (d));
  }

  if (excepts & FE_DIVBYZERO)
  {
    double d = 1.0, x = 0.0;
    __asm__ __volatile__ ("fdiv %1, %0" : "+d" (d) : "d" (x));
  }

  if (excepts & FE_INVALID)
  {
    double d = HUGE_VAL, x = 0.0;
    __asm__ __volatile__ ("fmul %1, %0" : "+d" (d) : "d" (x));
  }

  {
    /* Restore flag fields.  */
    fpu_control_t cw;
    _FPU_GETCW (cw);
    cw |= (excepts & FE_ALL_EXCEPT);
    _FPU_SETCW (cw);
  }

  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fpu_control_t temp;

  _FPU_GETCW (temp);
  temp |= (excepts & FE_ALL_EXCEPT);
  _FPU_SETCW (temp);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fpu_control_t temp;

  /* Get the current environment.  */
  _FPU_GETCW (temp);

  /* Set the desired exception mask.  */
  temp &= ~(excepts & FE_ALL_EXCEPT);
  temp |= (*flagp & excepts & FE_ALL_EXCEPT);

  /* Save state back to the FPU.  */
  _FPU_SETCW (temp);

  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  fpu_control_t temp;

  /* Get current exceptions.  */
  _FPU_GETCW (temp);

  return temp & excepts & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getround (void)
{
  fpu_control_t cw;

  /* Get control word.  */
  _FPU_GETCW (cw);

  return cw & 0x1;
}

static __always_inline int
fenv_setround (int round)
{
  fpu_control_t cw;

  if ((round & ~0x1) != 0)
    /* ROUND is no valid rounding mode.  */
    return 1;

  /* Get current state.  */
  _FPU_GETCW (cw);

  /* Set rounding bits.  */
  cw &= ~0x1;
  cw |= round;
  /* Set new state.  */
  _FPU_SETCW (cw);

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  fpu_control_t temp;
  _FPU_GETCW (temp);

  envp->__fpscr = temp;

  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  fpu_control_t temp;

  /* Store the environment.  */
  _FPU_GETCW (temp);
  envp->__fpscr = temp;

  /* Clear the status flags.  */
  temp &= ~FE_ALL_EXCEPT;

  /* Now set all exceptions to non-stop.  */
  temp &= ~(FE_ALL_EXCEPT << 5);

  _FPU_SETCW (temp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  if (envp == FE_DFL_ENV)
      _FPU_SETCW (_FPU_DEFAULT);
  else
    {
      fpu_control_t temp = envp->__fpscr;
      _FPU_SETCW (temp);
    }
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  fpu_control_t temp;

  _FPU_GETCW (temp);
  temp = (temp & FE_ALL_EXCEPT);

  /* Raise the saved exception. Incidentally for us the implementation
    defined format of the values in objects of type fexcept_t is the
    same as the ones specified using the FE_* constants. */
  fenv_setenv (envp);
  __feraiseexcept ((int) temp);

  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  _FPU_GETCW (*modep);
  return 0;
}

#define FPU_STATUS 0x3f07c

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  fpu_control_t fpscr;

  _FPU_GETCW (fpscr);
  fpscr &= FPU_STATUS;
  if (modep == FE_DFL_MODE)
    fpscr |= _FPU_DEFAULT;
  else
    fpscr |= *modep & ~FPU_STATUS;
  _FPU_SETCW (fpscr);

  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  fpu_control_t temp, old_exc;

  /* Get the current control register contents.  */
  _FPU_GETCW (temp);

  old_exc = (temp >> 5) & FE_ALL_EXCEPT;

  excepts &= FE_ALL_EXCEPT;

  temp &= ~(excepts << 5);
  _FPU_SETCW (temp);

  return old_exc;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  fpu_control_t temp, old_flag;

  /* Get current exceptions.  */
  _FPU_GETCW (temp);

  old_flag = (temp >> 5) & FE_ALL_EXCEPT;
  excepts &= FE_ALL_EXCEPT;

  temp |= excepts << 5;
  _FPU_SETCW (temp);

  return old_flag;
}

static __always_inline int
fenv_getexcept (void)
{
  fpu_control_t temp;

  /* Get current exceptions.  */
  _FPU_GETCW (temp);

  return (temp >> 5) & FE_ALL_EXCEPT;
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
