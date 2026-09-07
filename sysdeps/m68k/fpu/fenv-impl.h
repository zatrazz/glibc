/* Architecture-specific implementation of the <fenv.h> functions.
   m68k version.
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

#ifndef M68K_FENV_IMPL_H
#define M68K_FENV_IMPL_H 1

#include <fenv.h>
#include <fpu_control.h>
#include <float.h>
#include <math.h>

static __always_inline int
fenv_clearexcept (int excepts)
{
  fexcept_t fpsr;

  /* Mask out unsupported bits/exceptions.  */
  excepts &= FE_ALL_EXCEPT;

  /* Fetch the fpu status register.  */
  __asm__ __volatile__ ("fmove%.l %/fpsr,%0" : "=dm" (fpsr));

  /* Clear the relevant bits.  */
  fpsr &= ~excepts;

  /* Put the new data in effect.  */
  __asm__ __volatile__ ("fmove%.l %0,%/fpsr" : : "dm" (fpsr));

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  fexcept_t fpsr;

  /* Get the current exceptions.  */
  __asm__ __volatile__ ("fmove%.l %/fpsr,%0" : "=dm" (fpsr));

  *flagp = fpsr & excepts & FE_ALL_EXCEPT;

  /* Success.  */
  return 0;
}

#ifdef __mcoldfire__
/* Raise the exception MASK if it is set in EXCEPTS.  See the comment
   in fenv_raiseexcept for the mechanism.  */
static __always_inline void
fenv_raise_one_exception (int excepts, int mask)
{
  if (excepts & mask)
    {
      int fpsr;
      double unused;

      asm volatile ("fmove%.l %/fpsr,%0" : "=d" (fpsr));
      fpsr |= (mask << 6) | mask;
      asm volatile ("fmove%.l %0,%/fpsr" :: "d" (fpsr));
      asm volatile ("fmove%.l (%%sp),%0" : "=f" (unused));
    }
}
#endif

static __always_inline int
fenv_raiseexcept (int excepts)
{
#ifdef __mcoldfire__
  /* Raise exceptions represented by EXCEPTS.  But we must raise only one
     signal at a time.  It is important that if the overflow/underflow
     exception and the divide by zero exception are given at the same
     time, the overflow/underflow exception follows the divide by zero
     exception.

     The Coldfire FPU allows an exception to be raised by asserting
     the associated EXC bit and then executing an arbitrary arithmetic
     instruction.  fmove.l is classified as an arithmetic instruction
     and suffices for this purpose.

     We therefore raise an exception by setting both the EXC and AEXC
     bit associated with the exception (the former being 6 bits to the
     left of the latter) and then loading the longword at (%sp) into an
     FP register.  */

  fenv_raise_one_exception (excepts, FE_INVALID);
  fenv_raise_one_exception (excepts, FE_DIVBYZERO);
  fenv_raise_one_exception (excepts, FE_OVERFLOW);
  fenv_raise_one_exception (excepts, FE_UNDERFLOW);
  fenv_raise_one_exception (excepts, FE_INEXACT);
#else
  /* Raise exceptions represented by EXCEPTS.  But we must raise only one
     signal at a time.  It is important that if the overflow/underflow
     exception and the divide by zero exception are given at the same
     time, the overflow/underflow exception follows the divide by zero
     exception.  */

  /* First: invalid exception.  */
  if (excepts & FE_INVALID)
    {
      /* One example of an invalid operation is 0 * Infinity.  */
      double d = HUGE_VAL;
      __asm__ __volatile__ ("fmul%.s %#0r0,%0; fnop" : "=f" (d) : "0" (d));
    }

  /* Next: division by zero.  */
  if (excepts & FE_DIVBYZERO)
    {
      double d = 1.0;
      __asm__ __volatile__ ("fdiv%.s %#0r0,%0; fnop" : "=f" (d) : "0" (d));
    }

  /* Next: overflow.  */
  if (excepts & FE_OVERFLOW)
    {
      long double d = LDBL_MAX;

      __asm__ __volatile__ ("fmul%.x %0,%0; fnop" : "=f" (d) : "0" (d));
    }

  /* Next: underflow.  */
  if (excepts & FE_UNDERFLOW)
    {
      long double d = -LDBL_MAX;

      __asm__ __volatile__ ("fetox%.x %0; fnop" : "=f" (d) : "0" (d));
    }

  /* Last: inexact.  */
  if (excepts & FE_INEXACT)
    {
      long double d = 1.0;
      __asm__ __volatile__ ("fdiv%.s %#0r3,%0; fnop" : "=f" (d) : "0" (d));
    }
#endif

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fexcept_t fpsr;

  __asm__ __volatile__ ("fmove%.l %/fpsr,%0" : "=dm" (fpsr));
  fpsr |= excepts & FE_ALL_EXCEPT;
  __asm__ __volatile__ ("fmove%.l %0,%/fpsr" : : "dm" (fpsr));

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fexcept_t fpsr;

  /* Get the current status register.  */
  __asm__ __volatile__ ("fmove%.l %/fpsr,%0" : "=dm" (fpsr));

  /* Install the new exception bits in the Accrued Exception Byte.  */
  fpsr &= ~(excepts & FE_ALL_EXCEPT);
  fpsr |= *flagp & excepts & FE_ALL_EXCEPT;

  /* Store the new status register.  */
  __asm__ __volatile__ ("fmove%.l %0,%/fpsr" : : "dm" (fpsr));

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  fexcept_t fpsr;

  /* Get current exceptions.  */
  __asm__ __volatile__ ("fmove%.l %/fpsr,%0" : "=dm" (fpsr));

  return fpsr & excepts & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getround (void)
{
  int fpcr;

  __asm__ __volatile__ ("fmove%.l %!,%0" : "=dm" (fpcr));

  return fpcr & FE_UPWARD;
}

static __always_inline int
fenv_setround (int round)
{
  fexcept_t fpcr;

  if (round & ~FE_UPWARD)
    /* ROUND is no valid rounding mode.  */
    return 1;

  __asm__ __volatile__ ("fmove%.l %!,%0" : "=dm" (fpcr));
  fpcr &= ~FE_UPWARD;
  fpcr |= round;
  __asm__ __volatile__ ("fmove%.l %0,%!" : : "dm" (fpcr));

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
#ifdef __mcoldfire__
  __asm__ __volatile__ ("fmove%.l %/fpcr,%0" : "=dm" (envp->__control_register));
  __asm__ __volatile__ ("fmove%.l %/fpsr,%0" : "=dm" (envp->__status_register));
  __asm__ __volatile__ ("fmove%.l %/fpiar,%0" : "=dm" (envp->__instruction_address));
#else
  __asm__ __volatile__ ("fmovem%.l %/fpcr/%/fpsr/%/fpiar,%0" : "=m" (*envp));
#endif

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  fexcept_t fpcr, fpsr;

  /* Store the environment.  */
#ifdef __mcoldfire__
  __asm__ __volatile__ ("fmove%.l %/fpcr,%0" : "=dm" (envp->__control_register));
  __asm__ __volatile__ ("fmove%.l %/fpsr,%0" : "=dm" (envp->__status_register));
  __asm__ __volatile__ ("fmove%.l %/fpiar,%0" : "=dm" (envp->__instruction_address));
#else
  __asm__ __volatile__ ("fmovem%.l %/fpcr/%/fpsr/%/fpiar,%0" : "=m" (*envp));
#endif

  /* Now clear all exceptions.  */
  fpsr = envp->__status_register & ~FE_ALL_EXCEPT;
  __asm__ __volatile__ ("fmove%.l %0,%/fpsr" : : "dm" (fpsr));
  /* And set all exceptions to non-stop.  */
  fpcr = envp->__control_register & ~(FE_ALL_EXCEPT << 6);
  __asm__ __volatile__ ("fmove%.l %0,%!" : : "dm" (fpcr));

  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  fenv_t temp;

  /* Install the environment specified by ENVP.  But there are a few
     values which we do not want to come from the saved environment.
     Therefore, we get the current environment and replace the values
     we want to use from the environment specified by the parameter.  */
#ifdef __mcoldfire__
  __asm__ __volatile__ ("fmove%.l %/fpcr,%0" : "=dm" (temp.__control_register));
  __asm__ __volatile__ ("fmove%.l %/fpsr,%0" : "=dm" (temp.__status_register));
  __asm__ __volatile__ ("fmove%.l %/fpiar,%0" : "=dm" (temp.__instruction_address));
#else
  __asm__ __volatile__ ("fmovem%.l %/fpcr/%/fpsr/%/fpiar,%0" : "=m" (*&temp));
#endif

  temp.__status_register &= ~FE_ALL_EXCEPT;
  temp.__control_register &= ~((FE_ALL_EXCEPT << 6) | FE_UPWARD);
  if (envp == FE_DFL_ENV)
    ;
  else if (envp == FE_NOMASK_ENV)
    temp.__control_register |= FE_ALL_EXCEPT << 6;
  else
    {
      temp.__control_register |= (envp->__control_register
				  & ((FE_ALL_EXCEPT << 6) | FE_UPWARD));
      temp.__status_register |= envp->__status_register & FE_ALL_EXCEPT;
    }

#ifdef __mcoldfire__
  __asm__ __volatile__ ("fmove%.l %0,%/fpiar"
			:: "dm" (temp.__instruction_address));
  __asm__ __volatile__ ("fmove%.l %0,%/fpcr"
			:: "dm" (temp.__control_register));
  __asm__ __volatile__ ("fmove%.l %0,%/fpsr"
			:: "dm" (temp.__status_register));
#else
  __asm__ __volatile__ ("fmovem%.l %0,%/fpcr/%/fpsr/%/fpiar" : : "m" (*&temp));
#endif

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  fexcept_t fpsr;

  /* Save current exceptions.  */
  __asm__ __volatile__ ("fmove%.l %/fpsr,%0" : "=dm" (fpsr));
  fpsr &= FE_ALL_EXCEPT;

  /* Install new environment.  */
  fenv_setenv (envp);

  /* Raise the saved exception.  Incidentally for us the implementation
     defined format of the values in objects of type fexcept_t is the
     same as the ones specified using the FE_* constants.  */
  __feraiseexcept ((int) fpsr);

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
  unsigned int old_exc, new_exc;

  /* Get the current control register contents.  */
  __asm__ __volatile__ ("fmove%.l %!,%0" : "=dm" (new_exc));

  old_exc = (new_exc >> 6) & FE_ALL_EXCEPT;

  excepts &= FE_ALL_EXCEPT;

  new_exc &= ~(excepts << 6);
  __asm__ __volatile__ ("fmove%.l %0,%!" : : "dm" (new_exc));

  return old_exc;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  unsigned int new_exc, old_exc;

  /* Get the current control register contents.  */
  __asm__ __volatile__ ("fmove%.l %!,%0" : "=dm" (new_exc));

  old_exc = (new_exc >> 6) & FE_ALL_EXCEPT;

  excepts &= FE_ALL_EXCEPT;

  new_exc |= excepts << 6;
  __asm__ __volatile__ ("fmove%.l %0,%!" : : "dm" (new_exc));

  return old_exc;
}

static __always_inline int
fenv_getexcept (void)
{
  unsigned int exc;

  /* Get the current control register contents.  */
  __asm__ __volatile__ ("fmove%.l %!,%0" : "=dm" (exc));

  return (exc >> 6) & FE_ALL_EXCEPT;
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
