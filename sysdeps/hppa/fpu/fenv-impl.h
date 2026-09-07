/* Architecture-specific implementation of the <fenv.h> functions.
   HPPA version.
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

#ifndef HPPA_FENV_IMPL_H
#define HPPA_FENV_IMPL_H 1

#include <fenv.h>
#include <fpu_control.h>
#include <get-rounding-mode.h>
#include <float.h>
#include <math.h>
#include <string.h>

static __always_inline int
fenv_clearexcept (int excepts)
{
  union { unsigned long long l; unsigned int sw[2]; } s;

  /* Get the current status word. */
  __asm__ __volatile__ ("fstd %%fr0,0(%1)" : "=m" (s.l) : "r" (&s.l) : "%r0");
  /* Clear all the relevant bits. */
  s.sw[0] &= ~(((unsigned int) excepts & FE_ALL_EXCEPT) << 27);
  __asm__ __volatile__ ("fldd 0(%0),%%fr0" : : "r" (&s.l), "m" (s.l) : "%r0");

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  union { unsigned long long l; unsigned int sw[2]; } s;

  /* Get the current status word. */
  __asm__ __volatile__ ("fstd %%fr0,0(%1)	\n\t"
           "fldd 0(%1),%%fr0	\n\t"
	   : "=m" (s.l) : "r" (&s.l) : "%r0");

  *flagp = (s.sw[0] >> 27) & excepts & FE_ALL_EXCEPT;

  /* Success.  */
  return 0;
}

/* Please see section 10,
   page 10-5 "Delayed Trapping" in the PA-RISC 2.0 Architecture manual */

static __always_inline int
fenv_raiseexcept (int excepts)
{
  /* Raise exceptions represented by EXCEPTS.  But we must raise only one
     signal at a time.  It is important that if the overflow/underflow
     exception and the divide by zero exception are given at the same
     time, the overflow/underflow exception follows the divide by zero
     exception.  */

  /* We do these bits in assembly to be certain GCC doesn't optimize
     away something important, and so we can force delayed traps to
     occur. */

  /* We use "fldd 0(%%sr0,%%sp),%0" to flush the delayed exception */

  /* First: Invalid exception.  */
  if (excepts & FE_INVALID)
    {
      /* One example of an invalid operation is 0 * Infinity.  */
      double d = HUGE_VAL;
      __asm__ __volatile__ (
		"	fcpy,dbl %%fr0,%%fr22\n"
		"	fmpy,dbl %0,%%fr22,%0\n"
		"	fldd 0(%%sr0,%%sp),%0"
		: "+f" (d) : : "%fr22" );
    }

  /* Second: Division by zero.  */
  if (excepts & FE_DIVBYZERO)
    {
      double d = 1.0;
      __asm__ __volatile__ (
		"	fcpy,dbl %%fr0,%%fr22\n"
		"	fdiv,dbl %0,%%fr22,%0\n"
		"	fldd 0(%%sr0,%%sp),%0"
		: "+f" (d) : : "%fr22" );
    }

  /* Third: Overflow.  */
  if (excepts & FE_OVERFLOW)
    {
      double d = DBL_MAX;
      __asm__ __volatile__ (
		"	fadd,dbl %0,%0,%0\n"
		"	fldd 0(%%sr0,%%sp),%0"
		: "+f" (d) );
    }

  /* Fourth: Underflow.  */
  if (excepts & FE_UNDERFLOW)
    {
      double d = DBL_MIN;
      double e = 3.0;
      __asm__ __volatile__ (
		"	fdiv,dbl %0,%1,%0\n"
		"	fldd 0(%%sr0,%%sp),%0"
		: "+f" (d) : "f" (e) );
    }

  /* Fifth: Inexact */
  if (excepts & FE_INEXACT)
    {
      double d = M_PI;
      double e = 69.69;
      __asm__ __volatile__ (
		"	fdiv,dbl %0,%1,%%fr22\n"
		"	fcnvfxt,dbl,sgl %%fr22,%%fr22L\n"
		"	fldd 0(%%sr0,%%sp),%%fr22"
		: : "f" (d), "f" (e) : "%fr22" );
    }

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fpu_control_t fpsr;
  fpu_control_t fpsr_new;

  _FPU_GETCW (fpsr);
  excepts &= FE_ALL_EXCEPT;
  fpsr_new = fpsr | (excepts << _FPU_HPPA_SHIFT_FLAGS);
  if (fpsr != fpsr_new)
    _FPU_SETCW (fpsr_new);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fpu_control_t fpsr;
  fpu_control_t fpsr_new;

  /* Get the current status word. */
  _FPU_GETCW (fpsr);
  excepts &= FE_ALL_EXCEPT;

  /* Install new raised flags.  */
  fpsr_new = fpsr & ~(excepts << _FPU_HPPA_SHIFT_FLAGS);
  fpsr_new |= (*flagp & excepts) << _FPU_HPPA_SHIFT_FLAGS;

  /* Store the new status word.  */
  if (fpsr != fpsr_new)
    _FPU_SETCW (fpsr_new);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  union { unsigned long long l; unsigned int sw[2]; } s;

  /* Get the current status word. */
  __asm__ __volatile__ ("fstd %%fr0,0(%1)	\n\t"
           "fldd 0(%1),%%fr0	\n\t"
	   : "=m" (s.l) : "r" (&s.l));

  return (s.sw[0] >> 27) & excepts & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getround (void)
{
  return get_rounding_mode ();
}

static __always_inline int
fenv_setround (int round)
{
  union { unsigned long long l; unsigned int sw[2]; } s;

  if (round & ~FE_DOWNWARD)
    /* round is not a valid rounding mode. */
    return 1;

  /* Get the current status word. */
  __asm__ __volatile__ ("fstd %%fr0,0(%1)" : "=m" (s.l) : "r" (&s.l) : "%r0");
  s.sw[0] &= ~FE_DOWNWARD;
  s.sw[0] |= round & FE_DOWNWARD;
  __asm__ __volatile__ ("fldd 0(%0),%%fr0" : : "r" (&s.l), "m" (s.l) : "%r0");

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  unsigned long long buf[4], *bufptr = buf;

  __asm__ __volatile__ (
	   "fstd,ma %%fr0,8(%1)	\n\t"
	   "fldd -8(%1),%%fr0	\n\t"
	   : "=m" (buf), "+r" (bufptr) : : "%r0");
  memcpy(envp, buf, sizeof (*envp));
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  union { unsigned long long buf[4]; fenv_t env; } clear;
  unsigned long long *bufptr;

  /* Store the environment.  */
  bufptr = clear.buf;
  __asm__ __volatile__ (
	   "fstd %%fr0,0(%1)\n"
	   : "=m" (clear) : "r" (bufptr) : "%r0");
  memcpy (envp, &clear.env, sizeof (fenv_t));

  /* Clear exception queues */
  memset (clear.env.__exception, 0, sizeof (clear.env.__exception));
  /* And set all exceptions to non-stop.  */
  clear.env.__status_word &= ~FE_ALL_EXCEPT;
  /* Now clear all flags  */
  clear.env.__status_word &= ~(FE_ALL_EXCEPT << 27);

  /* Load the new environment. Note: fr0 must load last to enable T-bit.  */
  __asm__ __volatile__ (
	   "fldd 0(%0),%%fr0\n"
	   : : "r" (bufptr), "m" (clear) : "%r0");

  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  union { unsigned long long buf[4]; fenv_t env; } temp;
  unsigned long long *bufptr;

  /* Install the environment specified by ENVP.  But there are a few
     values which we do not want to come from the saved environment.
     Therefore, we get the current environment and replace the values
     we want to use from the environment specified by the parameter.  */
  bufptr = temp.buf;
  __asm__ __volatile__ (
	   "fstd %%fr0,0(%1)\n"
	   : "=m" (temp) : "r" (bufptr) : "%r0");

  temp.env.__status_word &= ~(FE_ALL_EXCEPT
			    | (FE_ALL_EXCEPT << 27)
			    | FE_DOWNWARD);
  if (envp == FE_DFL_ENV)
    temp.env.__status_word = 0;
  else if (envp == FE_NOMASK_ENV)
    temp.env.__status_word |= FE_ALL_EXCEPT;
  else
    temp.env.__status_word |= (envp->__status_word
			       & (FE_ALL_EXCEPT
				  | FE_DOWNWARD
				  | (FE_ALL_EXCEPT << 27)));

  /* Load the new environment. We use bufptr again since the
     initial asm has modified the value of the register and here
     we take advantage of that to load in reverse order so fr0
     is loaded last and T-Bit is enabled. */
  __asm__ __volatile__ (
	   "fldd 0(%1),%%fr0\n"
	   : : "m" (temp), "r" (bufptr) : "%r0" );

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  union { unsigned long long l; unsigned int sw[2]; } s;
  fenv_t temp;

  /* Get the current exception status */
  __asm__ __volatile__ ("fstd %%fr0,0(%1)	\n\t"
           "fldd 0(%1),%%fr0	\n\t"
	   : "=m" (s.l) : "r" (&s.l));

  /* Given environment with exception flags not cleared.  */
  if ((envp != FE_DFL_ENV) && (envp != FE_NOMASK_ENV))
    {
      memcpy(&temp, envp, sizeof (fenv_t));
      temp.__status_word |= s.sw[0] & (FE_ALL_EXCEPT << 27);
    }

  /* Default environment with exception flags not cleared.  */
  if (envp == FE_DFL_ENV)
    temp.__status_word = s.sw[0] & (FE_ALL_EXCEPT << 27);

  /* All traps enabled and current exception flags not cleared.  */
  if (envp == FE_NOMASK_ENV)
    temp.__status_word = (s.sw[0] & (FE_ALL_EXCEPT << 27)) | FE_ALL_EXCEPT;

  /* Install new environment.  */
  fenv_setenv (&temp);

  /* Raise exceptions.  */
  __feraiseexcept (temp.__status_word >> 27);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  _FPU_GETCW (*modep);
  return 0;
}

#define FPU_CONTROL_BITS (_FPU_HPPA_MASK_RM | 0x20 | _FPU_HPPA_MASK_INT)

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  fpu_control_t cw;
  _FPU_GETCW (cw);
  cw &= ~FPU_CONTROL_BITS;
  if (modep == FE_DFL_MODE)
    cw |= _FPU_DEFAULT;
  else
    cw |= *modep & FPU_CONTROL_BITS;
  _FPU_SETCW (cw);
  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  union { unsigned long long l; unsigned int sw[2]; } s;
  unsigned int old_exc;

  /* Get the current status word. */
  __asm__ __volatile__ ("fstd %%fr0,0(%1)" : "=m" (s.l) : "r" (&s.l) : "%r0");

  old_exc = s.sw[0] & FE_ALL_EXCEPT;

  s.sw[0] &= ~(excepts & FE_ALL_EXCEPT);
  __asm__ __volatile__ ("fldd 0(%0),%%fr0" : : "r" (&s.l), "m" (s.l) : "%r0");

  return old_exc;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  union { unsigned long long l; unsigned int sw[2]; } s;
  unsigned int old_exc;

  /* Get the current status word. */
  __asm__ __volatile__ ("fstd %%fr0,0(%1)" : "=m" (s.l) : "r" (&s.l) : "%r0");

  old_exc = s.sw[0] & FE_ALL_EXCEPT;

  s.sw[0] |= (excepts & FE_ALL_EXCEPT);
  __asm__ __volatile__ ("fldd 0(%0),%%fr0" : : "r" (&s.l), "m" (s.l) : "%r0");

  return old_exc;
}

static __always_inline int
fenv_getexcept (void)
{
  union { unsigned long long l; unsigned int sw[2]; } s;

  /* Get the current status word. */
  __asm__ __volatile__ ("fstd %%fr0,0(%1)	\n\t"
           "fldd 0(%1),%%fr0	\n\t"
	   : "=m" (s.l) : "r" (&s.l) : "%r0");

  return (s.sw[0] & FE_ALL_EXCEPT);
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
