/* Architecture-specific implementation of the <fenv.h> functions.
   i386 version.
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

#ifndef I386_FENV_IMPL_H
#define I386_FENV_IMPL_H 1

#include <fenv.h>
#include <assert.h>
#include <fpu_control.h>
#include <ldsodefs.h>
#include <math-inline-asm.h>

/* All exceptions, including the x86-specific "denormal operand"
   exception.  */
#define FE_ALL_EXCEPT_X86 (FE_ALL_EXCEPT | __FE_DENORM)

static __always_inline int
fenv_clearexcept (int excepts)
{
  fenv_t temp;

  /* Mask out unsupported bits/exceptions.  */
  excepts &= FE_ALL_EXCEPT;

  /* Bah, we have to clear selected exceptions.  Since there is no
     `fldsw' instruction we have to do it the hard way.  */
  __asm__ __volatile__ ("fnstenv %0" : "=m" (temp));

  /* Clear the relevant bits.  */
  temp.__status_word &= excepts ^ FE_ALL_EXCEPT;

  /* Put the new data in effect.  */
  __asm__ __volatile__ ("fldenv %0" : : "m" (temp));

  /* If the CPU supports SSE, we clear the MXCSR as well.  */
  if (CPU_FEATURE_USABLE (SSE))
    {
      unsigned int xnew_exc;

      /* Get the current MXCSR.  */
      stmxcsr_inline_asm (&xnew_exc);

      /* Clear the relevant bits.  */
      xnew_exc &= ~excepts;

      /* Put the new data in effect.  */
      ldmxcsr_inline_asm (&xnew_exc);
    }

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  fexcept_t temp;

  /* Get the current exceptions.  */
  __asm__ __volatile__ ("fnstsw %0" : "=m" (temp));

  *flagp = temp & excepts & FE_ALL_EXCEPT;

  /* If the CPU supports SSE, we clear the MXCSR as well.  */
  if (CPU_FEATURE_USABLE (SSE))
    {
      /* Get the current MXCSR.  */
      unsigned int sse_exc;
      stmxcsr_inline_asm (&sse_exc);

      *flagp |= sse_exc & excepts & FE_ALL_EXCEPT;
    }

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
    {
      /* One example of an invalid operation is 0.0 / 0.0.  */
      double d;
      __asm__ __volatile__ ("fldz; fdiv %%st, %%st(0); fwait" : "=t" (d));
      (void) &d;
    }

  /* Next: division by zero.  */
  if ((FE_DIVBYZERO & excepts) != 0)
    {
      double d;
      __asm__ __volatile__ ("fldz; fld1; fdivp %%st, %%st(1); fwait"
			    : "=t" (d));
      (void) &d;
    }

  /* Next: overflow.  */
  if ((FE_OVERFLOW & excepts) != 0)
    {
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
  /* The flags can be set in the 387 unit or in the SSE unit.  To set a flag,
     it is sufficient to do it in the SSE unit, because that is guaranteed to
     not trap.  However, on i386 CPUs that have only a 387 unit, set the flags
     in the 387, as long as this cannot trap.  */

  excepts &= FE_ALL_EXCEPT;

  if (CPU_FEATURE_USABLE (SSE))
    {
      unsigned int mxcsr;

      /* Get the control word of the SSE unit.  */
      stmxcsr_inline_asm (&mxcsr);

      /* Set relevant flags.  */
      mxcsr |= excepts;

      /* Put the new data in effect.  */
      ldmxcsr_inline_asm (&mxcsr);
    }
  else
    {
      fenv_t temp;

      /* Note: fnstenv masks all floating-point exceptions until the fldenv
	 or fldcw below.  */
      __asm__ __volatile__ ("fnstenv %0" : "=m" (temp));

      /* Set relevant flags.  */
      temp.__status_word |= excepts;

      if ((~temp.__control_word) & excepts)
	{
	  /* Setting the exception flags may trigger a trap (at the next
	     floating-point instruction, but that does not matter).
	     ISO C23 (7.6.4.4) does not allow it.  */
	  __asm__ volatile ("fldcw %0" : : "m" (temp.__control_word));
	  return -1;
	}

      /* Store the new status word (along with the rest of the environment).  */
      __asm__ __volatile__ ("fldenv %0" : : "m" (temp));
    }

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  /* The flags can be set in the 387 unit or in the SSE unit.  When we need to
     clear a flag, we need to do so in both units, due to the way fetestexcept
     is implemented.
     When we need to set a flag, it is sufficient to do it in the SSE unit,
     because that is guaranteed to not trap.  However, on i386 CPUs that have
     only a 387 unit, set the flags in the 387, as long as this cannot trap.  */

  fenv_t temp;

  excepts &= FE_ALL_EXCEPT;

  /* Get the current x87 FPU environment.  We have to do this since we
     cannot separately set the status word.
     Note: fnstenv masks all floating-point exceptions until the fldenv
     or fldcw below.  */
  __asm__ __volatile__ ("fnstenv %0" : "=m" (temp));

  if (CPU_FEATURE_USABLE (SSE))
    {
      unsigned int mxcsr;

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
    }
  else
    {
      /* Clear or set relevant flags.  */
      temp.__status_word ^= (temp.__status_word ^ *flagp) & excepts;

      if ((~temp.__control_word) & temp.__status_word & excepts)
	{
	  /* Setting the exception flags may trigger a trap (at the next
	     floating-point instruction, but that does not matter).
	     ISO C 23 § 7.6.4.5 does not allow it.  */
	  __asm__ volatile ("fldcw %0" : : "m" (temp.__control_word));
	  return -1;
	}

      /* Store the new status word (along with the rest of the environment).  */
      __asm__ __volatile__ ("fldenv %0" : : "m" (temp));
    }

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  short temp;
  unsigned int xtemp = 0;

  /* Get current exceptions.  */
  __asm__ __volatile__ ("fnstsw %0" : "=a" (temp));

  /* If the CPU supports SSE we test the MXCSR as well.  */
  if (CPU_FEATURE_USABLE (SSE))
    stmxcsr_inline_asm (&xtemp);

  return (temp | xtemp) & excepts & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getround (void)
{
  int cw;

  __asm__ __volatile__ ("fnstcw %0" : "=m" (cw));

  return cw & 0xc00;
}

static __always_inline int
fenv_setround (int round)
{
  unsigned short int cw;

  if ((round & ~0xc00) != 0)
    /* ROUND is no valid rounding mode.  */
    return 1;

  __asm__ __volatile__ ("fnstcw %0" : "=m" (cw));
  cw &= ~0xc00;
  cw |= round;
  __asm__ __volatile__ ("fldcw %0" : : "m" (cw));

  /* If the CPU supports SSE we set the MXCSR as well.  */
  if (CPU_FEATURE_USABLE (SSE))
    {
      unsigned int xcw;
      stmxcsr_inline_asm (&xcw);
      xcw &= ~0x6000;
      xcw |= round << 3;
      ldmxcsr_inline_asm (&xcw);
    }

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  __asm__ __volatile__ ("fnstenv %0" : "=m" (*envp));
  /* And load it right back since the processor changes the mask.
     Intel thought this opcode to be used in interrupt handlers which
     would block all exceptions.  */
  __asm__ __volatile__ ("fldenv %0" : : "m" (*envp));

  if (CPU_FEATURE_USABLE (SSE))
    stmxcsr_inline_asm (&envp->__eip);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  /* Store the environment.  Recall that fnstenv has a side effect of
     masking all exceptions.  Then clear all exceptions.  */
  __asm__ volatile ("fnstenv %0; fnclex" : "=m" (*envp));

  /* If the CPU supports SSE we set the MXCSR as well.  */
  if (CPU_FEATURE_USABLE (SSE))
    {
      unsigned int xwork;

      /* Get the current control word.  */
      stmxcsr_inline_asm (&envp->__eip);

      /* Set all exceptions to non-stop and clear them.  */
      xwork = (envp->__eip | 0x1f80) & ~0x3f;

      ldmxcsr_inline_asm (&xwork);
    }

  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  fenv_t temp;

  /* The memory block used by fstenv/fldenv has a size of 28 bytes.  */
  assert (sizeof (fenv_t) == 28);

  /* Install the environment specified by ENVP.  But there are a few
     values which we do not want to come from the saved environment.
     Therefore, we get the current environment and replace the values
     we want to use from the environment specified by the parameter.  */
  __asm__ __volatile__ ("fnstenv %0" : "=m" (temp));

  if (envp == FE_DFL_ENV)
    {
      temp.__control_word |= FE_ALL_EXCEPT_X86;
      temp.__control_word &= ~FE_TOWARDZERO;
      temp.__control_word |= _FPU_EXTENDED;
      temp.__status_word &= ~FE_ALL_EXCEPT_X86;
    }
  else if (envp == FE_NOMASK_ENV)
    {
      temp.__control_word &= ~(FE_ALL_EXCEPT | FE_TOWARDZERO);
      /* Keep the "denormal operand" exception masked.  */
      temp.__control_word |= __FE_DENORM;
      temp.__control_word |= _FPU_EXTENDED;
      temp.__status_word &= ~FE_ALL_EXCEPT_X86;
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
    }
  temp.__eip = 0;
  temp.__cs_selector = 0;
  temp.__opcode = 0;
  temp.__data_offset = 0;
  temp.__data_selector = 0;

  __asm__ __volatile__ ("fldenv %0" : : "m" (temp));

  if (CPU_FEATURE_USABLE (SSE))
    {
      unsigned int mxcsr;
      stmxcsr_inline_asm (&mxcsr);

      if (envp == FE_DFL_ENV)
	{
	  /* Clear SSE exceptions.  */
	  mxcsr &= ~FE_ALL_EXCEPT_X86;
	  /* Set mask for SSE MXCSR.  */
	  mxcsr |= (FE_ALL_EXCEPT_X86 << 7);
	  /* Set rounding to FE_TONEAREST.  */
	  mxcsr &= ~0x6000;
	  mxcsr |= (FE_TONEAREST << 3);
	  /* Clear the FZ and DAZ bits.  */
	  mxcsr &= ~0x8040;
	}
      else if (envp == FE_NOMASK_ENV)
	{
	  /* Clear SSE exceptions.  */
	  mxcsr &= ~FE_ALL_EXCEPT_X86;
	  /* Do not mask exceptions.  */
	  mxcsr &= ~(FE_ALL_EXCEPT << 7);
	  /* Keep the "denormal operand" exception masked.  */
	  mxcsr |= (__FE_DENORM << 7);
	  /* Set rounding to FE_TONEAREST.  */
	  mxcsr &= ~0x6000;
	  mxcsr |= (FE_TONEAREST << 3);
	  /* Clear the FZ and DAZ bits.  */
	  mxcsr &= ~0x8040;
	}
      else
	mxcsr = envp->__eip;

      ldmxcsr_inline_asm (&mxcsr);
    }

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  fexcept_t temp;
  unsigned int xtemp = 0;

  /* Save current exceptions.  */
  __asm__ __volatile__ ("fnstsw %0" : "=m" (temp));

  /* If the CPU supports SSE we test the MXCSR as well.  */
  if (CPU_FEATURE_USABLE (SSE))
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
  if (CPU_FEATURE_USABLE (SSE))
    stmxcsr_inline_asm (&modep->__mxcsr);
  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  fpu_control_t cw;
  if (modep == FE_DFL_MODE)
    cw = _FPU_DEFAULT;
  else
    cw = modep->__control_word;
  _FPU_SETCW (cw);
  if (CPU_FEATURE_USABLE (SSE))
    {
      unsigned int mxcsr;

      stmxcsr_inline_asm (&mxcsr);
      /* Preserve SSE exception flags but restore other state in
	 MXCSR.  */
      mxcsr &= FE_ALL_EXCEPT_X86;
      if (modep == FE_DFL_MODE)
	/* Default MXCSR state has all bits zero except for those
	   masking exceptions.  */
	mxcsr |= FE_ALL_EXCEPT_X86 << 7;
      else
	mxcsr |= modep->__mxcsr & ~FE_ALL_EXCEPT_X86;
      ldmxcsr_inline_asm (&mxcsr);
    }
  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  unsigned short int new_exc, old_exc;

  /* Get the current control word.  */
  __asm__ __volatile__ ("fstcw %0" : "=m" (new_exc));

  old_exc = (~new_exc) & FE_ALL_EXCEPT;

  excepts &= FE_ALL_EXCEPT;

  new_exc |= excepts;
  __asm__ __volatile__ ("fldcw %0" : : "m" (new_exc));

  /* If the CPU supports SSE we set the MXCSR as well.  */
  if (CPU_FEATURE_USABLE (SSE))
    {
      unsigned int xnew_exc;

      /* Get the current control word.  */
      stmxcsr_inline_asm (&xnew_exc);

      xnew_exc |= excepts << 7;

      ldmxcsr_inline_asm (&xnew_exc);
    }

  return old_exc;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  unsigned short int new_exc;
  unsigned short int old_exc;

  /* Get the current control word.  */
  __asm__ __volatile__ ("fstcw %0" : "=m" (new_exc));

  excepts &= FE_ALL_EXCEPT;
  old_exc = (~new_exc) & FE_ALL_EXCEPT;

  new_exc &= ~excepts;
  __asm__ __volatile__ ("fldcw %0" : : "m" (new_exc));

  /* If the CPU supports SSE we set the MXCSR as well.  */
  if (CPU_FEATURE_USABLE (SSE))
    {
      unsigned int xnew_exc;

      /* Get the current control word.  */
      stmxcsr_inline_asm (&xnew_exc);

      xnew_exc &= ~(excepts << 7);

      ldmxcsr_inline_asm (&xnew_exc);
    }

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
