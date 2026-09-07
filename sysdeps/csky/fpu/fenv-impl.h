/* Architecture-specific implementation of the <fenv.h> functions.
   C-SKY version.
   Copyright (C) 2018-2026 Free Software Foundation, Inc.
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

#ifndef CSKY_FENV_IMPL_H
#define CSKY_FENV_IMPL_H 1

#include <fenv.h>
#include <fenv_libc.h>
#include <fpu_control.h>
#include <float.h>
#include <math.h>

static __always_inline int
fenv_clearexcept (int excepts)
{
  int fpsr;

  /* Mask out unsupported bits/exceptions.  */
  excepts &= FE_ALL_EXCEPT;

  /* Read the complete control word.  */
  _FPU_GETFPSR (fpsr);

  /* Clear the relevant bits.  */
  fpsr &= ~(excepts | (excepts << CAUSE_SHIFT));

  /* Put the new data in effect.  */
  _FPU_SETFPSR (fpsr);

  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  fpu_control_t fpsr;

  _FPU_GETFPSR (fpsr);
  fpsr = fpsr >> CAUSE_SHIFT;
  *flagp = fpsr & excepts & FE_ALL_EXCEPT;

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
    /* Raise exceptions represented by EXCEPTS.  But we must raise only one
     signal at a time.  It is important that if the overflow/underflow
     exception and the divide by zero exception are given at the same
     time, the overflow/underflow exception follows the divide by zero
     exception.  */

# ifndef __csky_fpuv1__
    /* First: invalid exception.  */
    if (FE_INVALID & excepts)
    {
      /* One example of a invalid operation is 0 * Infinity.  */
      float x = HUGE_VALF, y = 0.0f;
      __asm__ __volatile__ ("fmuls %0, %0, %1" : "+v" (x) : "v" (y));
    }

    /* Next: division by zero.  */
    if (FE_DIVBYZERO & excepts)
    {
      float x = 1.0f, y = 0.0f;
      __asm__ __volatile__ ("fdivs %0, %0, %1" : "+v" (x) : "v" (y));
    }

    /* Next: overflow.  */
    if (FE_OVERFLOW & excepts)
    {
      float x = FLT_MAX;
      __asm__ __volatile__ ("fmuls %0, %0, %0" : "+v" (x));
    }
    /* Next: underflow.  */
    if (FE_UNDERFLOW & excepts)
    {
      float x = -FLT_MIN;

      __asm__ __volatile__ ("fmuls %0, %0, %0" : "+v" (x));
    }

    /* Last: inexact.  */
    if (FE_INEXACT & excepts)
    {
      float x = 1.0f, y = 3.0f;
      __asm__ __volatile__ ("fdivs %0, %0, %1" : "+v" (x) : "v" (y));
    }

    if (__FE_DENORMAL & excepts)
    {
      double x = 4.9406564584124654e-324;
      __asm__ __volatile__ ("fstod %0, %0" : "+v" (x));
    }
# else
     int tmp = 0;
    /* First: invalid exception.  */
    if (FE_INVALID & excepts)
    {
      /* One example of a invalid operation is 0 * Infinity.  */
      float x = HUGE_VALF, y = 0.0f;
      __asm__ __volatile__ ("fmuls %0, %0, %2, %1"
                    : "+f" (x), "+r"(tmp) : "f" (y));
    }

    /* Next: division by zero.  */
    if (FE_DIVBYZERO & excepts)
    {
      float x = 1.0f, y = 0.0f;
      __asm__ __volatile__ ("fdivs %0, %0, %2, %1"
                    : "+f" (x), "+r"(tmp) : "f" (y));
    }

    /* Next: overflow.  */
    if (FE_OVERFLOW & excepts)
    {
      float x = FLT_MAX, y = FLT_MAX;
      __asm__ __volatile__ ("fmuls %0, %0, %2, %1"
                    : "+f" (x), "+r"(tmp) : "f" (y));
    }

    /* Next: underflow.  */
    if (FE_UNDERFLOW & excepts)
    {
      float x = -FLT_MIN, y = -FLT_MIN;

      __asm__ __volatile__ ("fmuls %0, %0, %2, %1"
                    : "+f" (x), "+r"(tmp) : "f" (y));
    }

    /* Last: inexact.  */
    if (FE_INEXACT & excepts)
    {
      float x = 1.0f, y = 3.0f;
      __asm__ __volatile__ ("fdivs %0, %0, %2, %1"
                    : "+f" (x), "+r"(tmp) : "f" (y));
    }
# endif /* __csky_fpuv2__ */

    /* Success.  */
    return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fpu_control_t fpsr, new_fpsr;
  _FPU_GETFPSR (fpsr);
  new_fpsr = fpsr | ((excepts & FE_ALL_EXCEPT) << CAUSE_SHIFT);
  if (new_fpsr != fpsr)
    _FPU_SETFPSR (new_fpsr);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fpu_control_t temp;

  /* Get the current exceptions.  */
  _FPU_GETFPSR (temp);

  /* Make sure the flags we want restored are legal.  */
  excepts &= FE_ALL_EXCEPT;

  /* Now clear the bits called for, and copy them in from flagp.  Note that
     we ignore all non-flag bits from *flagp, so they don't matter.  */
  temp = ((temp >> CAUSE_SHIFT) & ~excepts) | (*flagp & excepts);
  temp = temp << CAUSE_SHIFT;

  _FPU_SETFPSR (temp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  fpu_control_t fpsr;

  _FPU_GETFPSR (fpsr);
  fpsr = fpsr >> CAUSE_SHIFT;
  return fpsr & excepts & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getround (void)
{
  unsigned int cw;

  /* Get control word.  */
  _FPU_GETCW (cw);

  return cw & __FE_ROUND_MASK;
}

static __always_inline int
fenv_setround (int round)
{
  fpu_control_t fpcr;

  _FPU_GETCW (fpcr);

  /* Set new rounding mode if different.  */
  if (__glibc_unlikely ((fpcr & FE_DOWNWARD) != round))
    _FPU_SETCW ((fpcr & ~FE_DOWNWARD) | round);
  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  unsigned int fpcr;
  unsigned int fpsr;

  _FPU_GETCW (fpcr);
  _FPU_GETFPSR (fpsr);
  envp->__fpcr = fpcr;
  envp->__fpsr = fpsr;

  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  fpu_control_t fpsr, fpcr;

  _FPU_GETCW (fpcr);
  envp->__fpcr = fpcr;

  _FPU_GETFPSR (fpsr);
  envp->__fpsr = fpsr;

  /* Now set all exceptions to non-stop.  */
  fpcr &= ~FE_ALL_EXCEPT;

  /* And clear all exception flags.  */
  fpsr &= ~(FE_ALL_EXCEPT << CAUSE_SHIFT);

  _FPU_SETFPSR (fpsr);

  _FPU_SETCW (fpcr);
  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  unsigned int fpcr;
  unsigned int fpsr;

  _FPU_GETCW (fpcr);
  _FPU_GETFPSR (fpsr);

  fpcr &= _FPU_RESERVED;
  fpsr &= _FPU_FPSR_RESERVED;

  if (envp == FE_DFL_ENV)
    {
      fpcr |= _FPU_DEFAULT;
      fpsr |= _FPU_FPSR_DEFAULT;
    }
  else if (envp == FE_NOMASK_ENV)
    {
      fpcr |= _FPU_FPCR_IEEE;
      fpsr |= _FPU_FPSR_IEEE;
    }
  else
    {
      fpcr |= envp->__fpcr & ~_FPU_RESERVED;
      fpsr |= envp->__fpsr & ~_FPU_FPSR_RESERVED;
    }

  _FPU_SETFPSR (fpsr);

  _FPU_SETCW (fpcr);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  int temp;

  /* Save current exceptions.  */
  _FPU_GETFPSR (temp);
  temp = (temp >> CAUSE_SHIFT) & FE_ALL_EXCEPT;
  /* Install new environment.  */
  fenv_setenv (envp);

  /* Raise the saved exception.  Incidentally for us the implementation
     defined format of the values in objects of type fexcept_t is the
     same as the ones specified using the FE_* constants.  */
  __feraiseexcept (temp);

  /* Success.  */
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
  femode_t mode;
  if (modep == FE_DFL_MODE)
    mode = _FPU_DEFAULT;
  else
    mode = *modep;
  _FPU_SETCW (mode);

  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  unsigned int new_exc, old_exc;

  /* Get the current control word.  */
  _FPU_GETCW (new_exc);

  old_exc = (new_exc & ENABLE_MASK) >> ENABLE_SHIFT;

  /* Get the except disable mask.  */
  excepts &= FE_ALL_EXCEPT;
  new_exc &= ~(excepts << ENABLE_SHIFT);

  /* Put the new data in effect.  */
  _FPU_SETCW (new_exc);

  return old_exc;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  unsigned int new_exc, old_exc;

  /* Get the current control word.  */
  _FPU_GETCW (new_exc);

  old_exc = (new_exc & ENABLE_MASK) >> ENABLE_SHIFT;

  excepts &= FE_ALL_EXCEPT;

  new_exc |= excepts << ENABLE_SHIFT;

  _FPU_SETCW (new_exc);

  return old_exc;
}

static __always_inline int
fenv_getexcept (void)
{
  unsigned int exc;

  /* Get the current control word.  */
  _FPU_GETCW (exc);

  return (exc & ENABLE_MASK) >> ENABLE_SHIFT;
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
