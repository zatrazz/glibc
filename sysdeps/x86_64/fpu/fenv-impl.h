/* Architecture-specific implementation of the <fenv.h> functions.
   x86_64 version.
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

#ifndef X86_64_FENV_IMPL_H
#define X86_64_FENV_IMPL_H 1

#include <fenv.h>
#include <fpu_control.h>
#include <math-inline-asm.h>

/* All exceptions, including the x86-specific "denormal operand"
   exception.  */
#define FE_ALL_EXCEPT_X86 (FE_ALL_EXCEPT | __FE_DENORM)

static __always_inline int
fenv_clearexcept (int excepts)
{
  fenv_t temp;
  unsigned int mxcsr;

  /* Mask out unsupported bits/exceptions.  */
  excepts &= FE_ALL_EXCEPT;

  /* Bah, we have to clear selected exceptions.  Since there is no
     `fldsw' instruction we have to do it the hard way.  */
  __asm__ __volatile__ ("fnstenv %0" : "=m" (temp));

  /* Clear the relevant bits.  */
  temp.__status_word &= excepts ^ FE_ALL_EXCEPT;

  /* Put the new data in effect.  */
  __asm__ __volatile__ ("fldenv %0" : : "m" (temp));

  /* And the same procedure for SSE.  */
  stmxcsr_inline_asm (&mxcsr);

  /* Clear the relevant bits.  */
  mxcsr &= ~excepts;

  /* And put them into effect.  */
  ldmxcsr_inline_asm (&mxcsr);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  fexcept_t temp;
  unsigned int mxscr;

  /* Get the current exceptions for the x87 FPU and SSE unit.  */
  __asm__ __volatile__ ("fnstsw %0" : "=m" (temp));
  stmxcsr_inline_asm (&mxscr);

  *flagp = (temp | mxscr) & FE_ALL_EXCEPT & excepts;

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  /* Raise exceptions represented by EXPECTS.  But we must raise only
     one signal at a time.  It is important that if the overflow/underflow
     exception and the inexact exception are given at the same time,
     the overflow/underflow exception follows the inexact exception.  */

  /* First: invalid exception.  */
  if ((FE_INVALID & excepts) != 0)
    /* One example of an invalid operation is 0.0 / 0.0.  */
    divss_inline_asm (0.0f, 0.0f);

  /* Next: division by zero.  */
  if ((FE_DIVBYZERO & excepts) != 0)
    divss_inline_asm (1.0f, 0.0f);

  /* Next: overflow.  */
  if ((FE_OVERFLOW & excepts) != 0)
    {
      /* XXX: Is it ok to only set the x87 FPU?  */
      /* There is no way to raise only the overflow flag.  Do it the
	 hard way.  */
      fenv_t temp;

      /* Bah, we have to clear selected exceptions.  Since there is no
	 `fldsw' instruction we have to do it the hard way.  */
      __asm__ __volatile__ ("fnstenv %0" : "=m" (temp));

      /* Set the relevant bits.  */
      temp.__status_word |= FE_OVERFLOW;

      /* Put the new data in effect.  */
      __asm__ __volatile__ ("fldenv %0" : : "m" (temp));

      /* And raise the exception.  */
      __asm__ __volatile__ ("fwait");
    }

  /* Next: underflow.  */
  if ((FE_UNDERFLOW & excepts) != 0)
    {
      /* XXX: Is it ok to only set the x87 FPU?  */
      /* There is no way to raise only the underflow flag.  Do it the
	 hard way.  */
      fenv_t temp;

      /* Bah, we have to clear selected exceptions.  Since there is no
	 `fldsw' instruction we have to do it the hard way.  */
      __asm__ __volatile__ ("fnstenv %0" : "=m" (temp));

      /* Set the relevant bits.  */
      temp.__status_word |= FE_UNDERFLOW;

      /* Put the new data in effect.  */
      __asm__ __volatile__ ("fldenv %0" : : "m" (temp));

      /* And raise the exception.  */
      __asm__ __volatile__ ("fwait");
    }

  /* Last: inexact.  */
  if ((FE_INEXACT & excepts) != 0)
    {
      /* XXX: Is it ok to only set the x87 FPU?  */
      /* There is no way to raise only the inexact flag.  Do it the
	 hard way.  */
      fenv_t temp;

      /* Bah, we have to clear selected exceptions.  Since there is no
	 `fldsw' instruction we have to do it the hard way.  */
      __asm__ __volatile__ ("fnstenv %0" : "=m" (temp));

      /* Set the relevant bits.  */
      temp.__status_word |= FE_INEXACT;

      /* Put the new data in effect.  */
      __asm__ __volatile__ ("fldenv %0" : : "m" (temp));

      /* And raise the exception.  */
      __asm__ __volatile__ ("fwait");
    }

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  unsigned int mxcsr;
  stmxcsr_inline_asm (&mxcsr);
  mxcsr |= excepts & FE_ALL_EXCEPT;
  ldmxcsr_inline_asm (&mxcsr);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  /* The flags can be set in the 387 unit or in the SSE unit.
     When we need to clear a flag, we need to do so in both units,
     due to the way fetestexcept() is implemented.
     When we need to set a flag, it is sufficient to do it in the SSE unit,
     because that is guaranteed to not trap.  */

  fenv_t temp;
  unsigned int mxcsr;

  excepts &= FE_ALL_EXCEPT;

  /* Get the current x87 FPU environment.  We have to do this since we
     cannot separately set the status word.  */
  __asm__ __volatile__ ("fnstenv %0" : "=m" (temp));

  /* Clear relevant flags.  */
  temp.__status_word &= ~(excepts & ~ *flagp);

  /* Store the new status word (along with the rest of the environment).  */
  __asm__ __volatile__ ("fldenv %0" : : "m" (temp));

  /* And now similarly for SSE.  */
  stmxcsr_inline_asm (&mxcsr);

  /* Clear or set relevant flags.  */
  mxcsr ^= (mxcsr ^ *flagp) & excepts;

  /* Put the new data in effect.  */
  ldmxcsr_inline_asm (&mxcsr);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  int temp;
  unsigned int mxscr;

  /* Get current exceptions.  */
  asm volatile ("fnstsw %0" : "=m" (temp));
  stmxcsr_inline_asm (&mxscr);

  return (temp | mxscr) & excepts & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getround (void)
{
  int cw;
  /* We only check the x87 FPU unit.  The SSE unit should be the same
     - and if it's not the same there's no way to signal it.  */

  __asm__ __volatile__ ("fnstcw %0" : "=m" (cw));

  return cw & 0xc00;
}

static __always_inline int
fenv_setround (int round)
{
  unsigned short int cw;
  unsigned int mxcsr;

  if ((round & ~0xc00) != 0)
    /* ROUND is no valid rounding mode.  */
    return 1;

  /* First set the x87 FPU.  */
  asm volatile ("fnstcw %0" : "=m" (cw));
  cw &= ~0xc00;
  cw |= round;
  asm volatile ("fldcw %0" : : "m" (cw));

  /* And now the MSCSR register for SSE, the precision is at different bit
     positions in the different units, we need to shift it 3 bits.  */
  stmxcsr_inline_asm (&mxcsr);
  mxcsr &= ~ 0x6000;
  mxcsr |= round << 3;
  ldmxcsr_inline_asm (&mxcsr);

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  asm volatile ("fnstenv %0\n"
		/* fnstenv changes the exception mask, so load back the
		   stored environment.  */
		"fldenv %0"
		: "=m" (*envp));
  stmxcsr_inline_asm (&envp->__mxcsr);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  unsigned int mxcsr;

  /* Store the environment.  Recall that fnstenv has a side effect of
     masking all exceptions.  Then clear all exceptions.  */
  asm volatile ("fnstenv %0" : "=m" (*envp));
  stmxcsr_inline_asm (&envp->__mxcsr);
  asm volatile ("fnclex" : "=m" (*envp));

  /* Set the SSE MXCSR register.  */
  mxcsr = (envp->__mxcsr | 0x1f80) & ~0x3f;
  ldmxcsr_inline_asm (&mxcsr);

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
  asm volatile ("fnstenv %0" : "=m" (temp));
  stmxcsr_inline_asm (&temp.__mxcsr);

  if (envp == FE_DFL_ENV)
    {
      temp.__control_word |= FE_ALL_EXCEPT_X86;
      temp.__control_word &= ~FE_TOWARDZERO;
      temp.__control_word |= _FPU_EXTENDED;
      temp.__status_word &= ~FE_ALL_EXCEPT_X86;
      temp.__eip = 0;
      temp.__cs_selector = 0;
      temp.__opcode = 0;
      temp.__data_offset = 0;
      temp.__data_selector = 0;
      /* Clear SSE exceptions.  */
      temp.__mxcsr &= ~FE_ALL_EXCEPT_X86;
      /* Set mask for SSE MXCSR.  */
      temp.__mxcsr |= (FE_ALL_EXCEPT_X86 << 7);
      /* Set rounding to FE_TONEAREST.  */
      temp.__mxcsr &= ~ 0x6000;
      temp.__mxcsr |= (FE_TONEAREST << 3);
      /* Clear the FZ and DAZ bits.  */
      temp.__mxcsr &= ~0x8040;
    }
  else if (envp == FE_NOMASK_ENV)
    {
      temp.__control_word &= ~(FE_ALL_EXCEPT | FE_TOWARDZERO);
      /* Keep the "denormal operand" exception masked.  */
      temp.__control_word |= __FE_DENORM;
      temp.__control_word |= _FPU_EXTENDED;
      temp.__status_word &= ~FE_ALL_EXCEPT_X86;
      temp.__eip = 0;
      temp.__cs_selector = 0;
      temp.__opcode = 0;
      temp.__data_offset = 0;
      temp.__data_selector = 0;
      /* Clear SSE exceptions.  */
      temp.__mxcsr &= ~FE_ALL_EXCEPT_X86;
      /* Set mask for SSE MXCSR.  */
      /* Set rounding to FE_TONEAREST.  */
      temp.__mxcsr &= ~ 0x6000;
      temp.__mxcsr |= (FE_TONEAREST << 3);
      /* Do not mask exceptions.  */
      temp.__mxcsr &= ~(FE_ALL_EXCEPT << 7);
      /* Keep the "denormal operand" exception masked.  */
      temp.__mxcsr |= (__FE_DENORM << 7);
      /* Clear the FZ and DAZ bits.  */
      temp.__mxcsr &= ~0x8040;
    }
  else
    {
      temp.__control_word &= ~(FE_ALL_EXCEPT_X86
			       | FE_TOWARDZERO
			       | _FPU_EXTENDED);
      temp.__control_word |= (envp->__control_word
			      & (FE_ALL_EXCEPT_X86
				 | FE_TOWARDZERO
				 | _FPU_EXTENDED));
      temp.__status_word &= ~FE_ALL_EXCEPT_X86;
      temp.__status_word |= envp->__status_word & FE_ALL_EXCEPT_X86;
      temp.__eip = envp->__eip;
      temp.__cs_selector = envp->__cs_selector;
      temp.__opcode = envp->__opcode;
      temp.__data_offset = envp->__data_offset;
      temp.__data_selector = envp->__data_selector;
      temp.__mxcsr = envp->__mxcsr;
    }

  asm volatile ("fldenv %0" : : "m" (temp));
  ldmxcsr_inline_asm (&temp.__mxcsr);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  fexcept_t temp;
  unsigned int xtemp;

  /* Save current exceptions.  */
  asm volatile ("fnstsw %0" : "=m" (temp));
  stmxcsr_inline_asm (&xtemp);
  temp = (temp | xtemp) & FE_ALL_EXCEPT;

  /* Install new environment.  */
  fenv_setenv (envp);

  /* Raise the saved exception.  Incidentally for us the implementation
     defined format of the values in objects of type fexcept_t is the
     same as the ones specified using the FE_* constants.  */
  __feraiseexcept ((int) temp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  _FPU_GETCW (modep->__control_word);
  stmxcsr_inline_asm (&modep->__mxcsr);
  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  fpu_control_t cw;
  unsigned int mxcsr;

  stmxcsr_inline_asm (&mxcsr);
  /* Preserve SSE exception flags but restore other state in
     MXCSR.  */
  mxcsr &= FE_ALL_EXCEPT_X86;
  if (modep == FE_DFL_MODE)
    {
      cw = _FPU_DEFAULT;
      /* Default MXCSR state has all bits zero except for those
	 masking exceptions.  */
      mxcsr |= FE_ALL_EXCEPT_X86 << 7;
    }
  else
    {
      cw = modep->__control_word;
      mxcsr |= modep->__mxcsr & ~FE_ALL_EXCEPT_X86;
    }
  _FPU_SETCW (cw);
  ldmxcsr_inline_asm (&mxcsr);
  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  unsigned short int new_exc, old_exc;
  unsigned int new;

  excepts &= FE_ALL_EXCEPT;

  /* Get the current control word of the x87 FPU.  */
  __asm__ __volatile__ ("fstcw %0" : "=m" (new_exc));

  old_exc = (~new_exc) & FE_ALL_EXCEPT;

  new_exc |= excepts;
  __asm__ __volatile__ ("fldcw %0" : : "m" (new_exc));

  /* And now the same for the SSE MXCSR register.  */
  stmxcsr_inline_asm (&new);

  /* The SSE exception masks are shifted by 7 bits.  */
  new |= excepts << 7;
  ldmxcsr_inline_asm (&new);

  return old_exc;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  unsigned short int new_exc, old_exc;
  unsigned int new;

  excepts &= FE_ALL_EXCEPT;

  /* Get the current control word of the x87 FPU.  */
  __asm__ __volatile__ ("fstcw %0" : "=m" (new_exc));

  old_exc = (~new_exc) & FE_ALL_EXCEPT;

  new_exc &= ~excepts;
  __asm__ __volatile__ ("fldcw %0" : : "m" (new_exc));

  /* And now the same for the SSE MXCSR register.  */
  stmxcsr_inline_asm (&new);

  /* The SSE exception masks are shifted by 7 bits.  */
  new &= ~(excepts << 7);
  ldmxcsr_inline_asm (&new);

  return old_exc;
}

static __always_inline int
fenv_getexcept (void)
{
  unsigned short int exc;

  /* Get the current control word.  */
  __asm__ __volatile__ ("fstcw %0" : "=m" (exc));

  return (~exc) & FE_ALL_EXCEPT;
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
