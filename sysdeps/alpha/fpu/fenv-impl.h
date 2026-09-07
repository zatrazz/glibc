/* Architecture-specific implementation of the <fenv.h> functions.
   Alpha version.
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
   License along with the GNU C Library.  If not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef ALPHA_FENV_IMPL_H
#define ALPHA_FENV_IMPL_H 1

#include <fenv.h>
#include <fenv_libc.h>
#include <sysdep.h>
#include <kernel_sysinfo.h>

static __always_inline int
fenv_clearexcept (int excepts)
{
  unsigned long int swcr;

  /* Get the current state.  */
  swcr = __ieee_get_fp_control ();

  /* Clear the relevant bits.  */
  swcr &= ~((unsigned long int) excepts & SWCR_STATUS_MASK);

  /* Put the new state in effect.  */
  __ieee_set_fp_control (swcr);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  unsigned long int tmp;

  /* Get the current state.  */
  tmp = __ieee_get_fp_control();

  /* Return that portion that corresponds to the requested exceptions. */
  *flagp = tmp & excepts & SWCR_STATUS_MASK;

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  /* The kernel raises the exceptions: it records them in the software
     status word and delivers SIGFPE for those that are enabled to trap.
     The value is passed by address.  */
  unsigned long int exc = excepts;
  long int ret = INTERNAL_SYSCALL_CALL (osf_setsysinfo,
					SSI_IEEE_RAISE_EXCEPTION, &exc);

  /* Here in libm we can't set errno, nor is it clear that we'd want to
     anyway.  All we're required to do is return non-zero on error.  */
  return INTERNAL_SYSCALL_ERROR_P (ret);
}

static __always_inline int
fenv_setexcept (int excepts)
{
  unsigned long int tmp;

  tmp = __ieee_get_fp_control ();
  tmp |= excepts & SWCR_STATUS_MASK;
  __ieee_set_fp_control (tmp);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  unsigned long int tmp;

  /* Get the current exception state.  */
  tmp = __ieee_get_fp_control ();

  /* Set all the bits that were called for.  */
  tmp ^= (tmp ^ *flagp) & excepts & SWCR_STATUS_MASK;

  /* And store it back.  */
  __ieee_set_fp_control (tmp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  unsigned long tmp;

  /* Get current exceptions.  */
  tmp = __ieee_get_fp_control();

  return tmp & excepts & SWCR_STATUS_MASK;
}

static __always_inline int
fenv_getround (void)
{
  unsigned long fpcr;

  __asm__ __volatile__("excb; mf_fpcr %0" : "=f"(fpcr));

  return (fpcr >> FPCR_ROUND_SHIFT) & 3;
}

static __always_inline int
fenv_setround (int round)
{
  unsigned long fpcr;

  if (round & ~3)
    return 1;

  /* Get the current state.  */
  __asm__ __volatile__("excb; mf_fpcr %0" : "=f"(fpcr));

  /* Set the relevant bits.  */
  fpcr = ((fpcr & ~FPCR_ROUND_MASK)
	  | ((unsigned long)round << FPCR_ROUND_SHIFT));

  /* Put the new state in effect.  */
  __asm__ __volatile__("mt_fpcr %0; excb" : : "f"(fpcr));

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  unsigned long int fpcr;
  unsigned long int swcr;

  /* Get status from software and hardware.  Note that we don't need an
     excb because the callsys is an implied trap barrier.  */
  swcr = __ieee_get_fp_control ();
  __asm__ __volatile__ ("mf_fpcr %0" : "=f" (fpcr));

  /* Merge the two bits of information.  */
  *envp = ((fpcr & FPCR_ROUND_MASK) | (swcr & SWCR_ALL_MASK));

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  /* Save the current state.  */
  fenv_getenv (envp);

  /* Clear all exception status bits and exception enable bits.  */
  __ieee_set_fp_control(*envp & SWCR_MAP_MASK);

  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  unsigned long int fpcr;
  fenv_t env;

  /* Magic encoding of default values: high bit set (never possible for a
     user-space address) is not indirect.  And we don't even have to get
     rid of it since we mask things around just below.  */
  if ((long int) envp >= 0)
    env = *envp;
  else
    env = (unsigned long int) envp;

  /* Reset the rounding mode with the hardware fpcr.  Note that the following
     system call is an implied trap barrier for our modification.  */
  __asm__ __volatile__ ("excb; mf_fpcr %0" : "=f" (fpcr));
  fpcr = (fpcr & ~FPCR_ROUND_MASK) | (env & FPCR_ROUND_MASK);
  __asm__ __volatile__ ("mt_fpcr %0" : : "f" (fpcr));

  /* Reset the exception status and mask with the kernel's FP code.  */
  __ieee_set_fp_control (env & SWCR_ALL_MASK);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  unsigned long int tmp;

  /* Get the current exception state.  */
  tmp = __ieee_get_fp_control ();

  /* Install new environment.  */
  fenv_setenv (envp);

  /* Raise the saved exception.  Incidentally for us the implementation
     defined format of the values in objects of type fexcept_t is the
     same as the ones specified using the FE_* constants.  */
  __feraiseexcept (tmp & SWCR_STATUS_MASK);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  unsigned long int fpcr;
  unsigned long int swcr;

  /* As in fegetenv.  */
  swcr = __ieee_get_fp_control ();
  __asm__ __volatile__ ("mf_fpcr %0" : "=f" (fpcr));
  *modep = ((fpcr & FPCR_ROUND_MASK) | (swcr & SWCR_ALL_MASK));

  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  unsigned long int fpcr;
  unsigned long int swcr;
  femode_t mode;

  /* As in fesetenv.  */
  if ((long int) modep >= 0)
    mode = *modep;
  else
    mode = (unsigned long int) modep;

  __asm__ __volatile__ ("excb; mf_fpcr %0" : "=f" (fpcr));
  fpcr = (fpcr & ~FPCR_ROUND_MASK) | (mode & FPCR_ROUND_MASK);
  __asm__ __volatile__ ("mt_fpcr %0" : : "f" (fpcr));

  swcr = __ieee_get_fp_control ();
  swcr = ((mode & SWCR_ALL_MASK & ~SWCR_STATUS_MASK)
	  | (swcr & SWCR_STATUS_MASK));
  __ieee_set_fp_control (swcr);

  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  unsigned long int new_exc, old_exc;

  new_exc = __ieee_get_fp_control ();

  old_exc = (new_exc & SWCR_ENABLE_MASK) << SWCR_ENABLE_SHIFT;
  new_exc &= ~((excepts >> SWCR_ENABLE_SHIFT) & SWCR_ENABLE_MASK);

  __ieee_set_fp_control (new_exc);

  return old_exc;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  unsigned long int new_exc, old_exc;

  new_exc = __ieee_get_fp_control ();

  old_exc = (new_exc & SWCR_ENABLE_MASK) << SWCR_ENABLE_SHIFT;
  new_exc |= (excepts >> SWCR_ENABLE_SHIFT) & SWCR_ENABLE_MASK;

  __ieee_set_fp_control (new_exc);

  return old_exc;
}

static __always_inline int
fenv_getexcept (void)
{
  unsigned long int exc;

  exc = __ieee_get_fp_control ();

  return (exc & SWCR_ENABLE_MASK) << SWCR_ENABLE_SHIFT;
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
