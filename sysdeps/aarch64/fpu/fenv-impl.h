/* Architecture-specific implementation of the <fenv.h> functions.
   AArch64 version.
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

#ifndef AARCH64_FENV_IMPL_H
#define AARCH64_FENV_IMPL_H 1

#include <fenv.h>
#include <fpu_control.h>
#include <get-rounding-mode.h>
#include <float.h>
#include <stdint.h>

/* The inline functions used both by the fenv functions and by the libm
   internal <fenv_private.h> hooks.  */

static __always_inline void
libc_fesetround_aarch64 (int round)
{
  fpu_control_t fpcr;

  _FPU_GETCW (fpcr);

  /* Check whether rounding modes are different.  */
  round = (fpcr ^ round) & _FPU_FPCR_RM_MASK;

  /* Set new rounding mode if different.  */
  if (__glibc_unlikely (round != 0))
    _FPU_SETCW (fpcr ^ round);
}

static __always_inline void
libc_fesetenv_aarch64 (const fenv_t *envp)
{
  fpu_control_t fpcr;
  fpu_control_t new_fpcr;

  _FPU_GETCW (fpcr);
  new_fpcr = envp->__fpcr;

  if (__glibc_unlikely (fpcr != new_fpcr))
    _FPU_SETCW (new_fpcr);

  _FPU_SETFPSR (envp->__fpsr);
}

static __always_inline int
libc_feupdateenv_test_aarch64 (const fenv_t *envp, int ex)
{
  fpu_control_t fpcr;
  fpu_control_t new_fpcr;
  fpu_fpsr_t fpsr;
  fpu_fpsr_t new_fpsr;
  int excepts;

  _FPU_GETCW (fpcr);
  _FPU_GETFPSR (fpsr);

  /* Merge current exception flags with the saved fenv.  */
  excepts = fpsr & FE_ALL_EXCEPT;
  new_fpcr = envp->__fpcr;
  new_fpsr = envp->__fpsr | excepts;

  if (__glibc_unlikely (fpcr != new_fpcr))
    _FPU_SETCW (new_fpcr);

  if (fpsr != new_fpsr)
    _FPU_SETFPSR (new_fpsr);

  /* Raise the exceptions if enabled in the new FP state.  */
  if (__glibc_unlikely (excepts & (new_fpcr >> FE_EXCEPT_SHIFT)))
    __feraiseexcept (excepts);

  return excepts & ex;
}

static __always_inline void
libc_feupdateenv_aarch64 (const fenv_t *envp)
{
  libc_feupdateenv_test_aarch64 (envp, 0);
}

static __always_inline int
fenv_clearexcept (int excepts)
{
  fpu_fpsr_t fpsr;
  fpu_fpsr_t fpsr_new;

  excepts &= FE_ALL_EXCEPT;

  _FPU_GETFPSR (fpsr);
  fpsr_new = fpsr & ~excepts;

  if (fpsr != fpsr_new)
    _FPU_SETFPSR (fpsr_new);

  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  fpu_fpsr_t fpsr;
  _FPU_GETFPSR (fpsr);
  *flagp = fpsr & excepts & FE_ALL_EXCEPT;
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  uint64_t fpsr;
  const float fp_zero = 0.0;
  const float fp_one = 1.0;
  const float fp_max = FLT_MAX;
  const float fp_min = FLT_MIN;
  const float fp_1e32 = 1.0e32f;
  const float fp_two = 2.0;
  const float fp_three = 3.0;

  /* Raise exceptions represented by EXCEPTS.  But we must raise only
     one signal at a time.  It is important that if the OVERFLOW or
     UNDERFLOW exception and the inexact exception are given at the
     same time, the OVERFLOW or UNDERFLOW exception precedes the
     INEXACT exception.

     After each exception we read from the FPSR, to force the
     exception to be raised immediately.  */

  if (FE_INVALID & excepts)
    __asm__ __volatile__ (
			  "ldr	s0, %1\n\t"
			  "fdiv	s0, s0, s0\n\t"
			  "mrs	%0, fpsr" : "=r" (fpsr)
			  : "m" (fp_zero)
			  : "d0");

  if (FE_DIVBYZERO & excepts)
    __asm__ __volatile__ (
			  "ldr	s0, %1\n\t"
			  "ldr	s1, %2\n\t"
			  "fdiv	s0, s0, s1\n\t"
			  "mrs	%0, fpsr" : "=r" (fpsr)
			  : "m" (fp_one), "m" (fp_zero)
			  : "d0", "d1");

  if (FE_OVERFLOW & excepts)
    /* There's no way to raise overflow without also raising inexact.  */
    __asm__ __volatile__ (
			  "ldr	s0, %1\n\t"
			  "ldr	s1, %2\n\t"
			  "fadd s0, s0, s1\n\t"
			  "mrs	%0, fpsr" : "=r" (fpsr)
			  : "m" (fp_max), "m" (fp_1e32)
			  : "d0", "d1");

  if (FE_UNDERFLOW & excepts)
    __asm__ __volatile__ (
			  "ldr	s0, %1\n\t"
			  "ldr	s1, %2\n\t"
			  "fdiv s0, s0, s1\n\t"
			  "mrs	%0, fpsr" : "=r" (fpsr)
			  : "m" (fp_min), "m" (fp_three)
			  : "d0", "d1");

  if (FE_INEXACT & excepts)
    __asm__ __volatile__ (
			  "ldr	s0, %1\n\t"
			  "ldr	s1, %2\n\t"
			  "fdiv s0, s0, s1\n\t"
			  "mrs	%0, fpsr" : "=r" (fpsr)
			  : "m" (fp_two), "m" (fp_three)
			  : "d0", "d1");

  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fpu_fpsr_t fpsr;
  fpu_fpsr_t fpsr_new;

  _FPU_GETFPSR (fpsr);
  fpsr_new = fpsr | (excepts & FE_ALL_EXCEPT);
  if (fpsr != fpsr_new)
    _FPU_SETFPSR (fpsr_new);
  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fpu_fpsr_t fpsr;
  fpu_fpsr_t fpsr_new;

  /* Get the current environment.  */
  _FPU_GETFPSR (fpsr);
  excepts &= FE_ALL_EXCEPT;

  /* Set the desired exception mask.  */
  fpsr_new = fpsr & ~excepts;
  fpsr_new |= *flagp & excepts;

  /* Save state back to the FPU.  */
  if (fpsr != fpsr_new)
    _FPU_SETFPSR (fpsr_new);

  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  fpu_fpsr_t fpsr;

  _FPU_GETFPSR (fpsr);
  return fpsr & excepts & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getround (void)
{
  return get_rounding_mode ();
}

static __always_inline int
fenv_setround (int round)
{
  if (round & ~_FPU_FPCR_RM_MASK)
    return 1;

  libc_fesetround_aarch64 (round);
  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  fpu_control_t fpcr;
  fpu_fpsr_t fpsr;
  _FPU_GETCW (fpcr);
  _FPU_GETFPSR (fpsr);
  envp->__fpcr = fpcr;
  envp->__fpsr = fpsr;
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  fpu_control_t fpcr;
  fpu_control_t new_fpcr;
  fpu_fpsr_t fpsr;
  fpu_fpsr_t new_fpsr;

  _FPU_GETCW (fpcr);
  _FPU_GETFPSR (fpsr);
  envp->__fpcr = fpcr;
  envp->__fpsr = fpsr;

  /* Clear exception flags and set all exceptions to non-stop.  */
  new_fpcr = fpcr & ~(FE_ALL_EXCEPT << FE_EXCEPT_SHIFT);
  new_fpsr = fpsr & ~FE_ALL_EXCEPT;

  if (__glibc_unlikely (new_fpcr != fpcr))
    _FPU_SETCW (new_fpcr);

  if (new_fpsr != fpsr)
    _FPU_SETFPSR (new_fpsr);
  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  fpu_control_t fpcr;
  fpu_control_t fpcr_new;
  fpu_control_t updated_fpcr;
  fpu_fpsr_t fpsr;
  fpu_fpsr_t fpsr_new;

  if ((envp != FE_DFL_ENV) && (envp != FE_NOMASK_ENV))
    {
      /* The new FPCR/FPSR are valid, so don't merge the reserved flags.  */
      libc_fesetenv_aarch64 (envp);
      return 0;
    }

  _FPU_GETCW (fpcr);
  _FPU_GETFPSR (fpsr);
  fpcr_new = fpcr & _FPU_RESERVED;
  fpsr_new = fpsr & _FPU_FPSR_RESERVED;

  if (envp == FE_DFL_ENV)
    {
      fpcr_new |= _FPU_DEFAULT;
      fpsr_new |= _FPU_FPSR_DEFAULT;
    }
  else
    {
      fpcr_new |= _FPU_FPCR_IEEE;
      fpsr_new |= _FPU_FPSR_IEEE;
    }

  _FPU_SETFPSR (fpsr_new);

  if (fpcr != fpcr_new)
    {
      _FPU_SETCW (fpcr_new);

      /* Trapping exceptions are optional in AArch64; the relevant enable
	 bits in FPCR are RES0 hence the absence of support can be detected
	 by reading back the FPCR and comparing with the required value.  */
      _FPU_GETCW (updated_fpcr);

      return fpcr_new & ~updated_fpcr;
    }

  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  fpu_control_t fpcr;
  fpu_control_t fpcr_new;
  fpu_control_t updated_fpcr;
  fpu_fpsr_t fpsr;
  fpu_fpsr_t fpsr_new;
  int excepts;

  if ((envp != FE_DFL_ENV) && (envp != FE_NOMASK_ENV))
    {
      libc_feupdateenv_aarch64 (envp);
      return 0;
    }

  _FPU_GETCW (fpcr);
  _FPU_GETFPSR (fpsr);
  excepts = fpsr & FE_ALL_EXCEPT;

  fpcr_new = fpcr & _FPU_RESERVED;
  fpsr_new = fpsr & (_FPU_FPSR_RESERVED | FE_ALL_EXCEPT);

  if (envp == FE_DFL_ENV)
    {
      fpcr_new |= _FPU_DEFAULT;
      fpsr_new |= _FPU_FPSR_DEFAULT;
    }
  else
    {
      fpcr_new |= _FPU_FPCR_IEEE;
      fpsr_new |= _FPU_FPSR_IEEE;
    }

  _FPU_SETFPSR (fpsr_new);

  if (fpcr != fpcr_new)
    {
      _FPU_SETCW (fpcr_new);

      /* Trapping exceptions are optional in AArch64; the relevant enable
	 bits in FPCR are RES0 hence the absence of support can be detected
	 by reading back the FPCR and comparing with the required value.  */
      _FPU_GETCW (updated_fpcr);

      if (fpcr_new & ~updated_fpcr)
        return 1;
    }

  if (excepts & (fpcr_new >> FE_EXCEPT_SHIFT))
    return __feraiseexcept (excepts);

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
  fpu_control_t fpcr, fpcr_new;

  _FPU_GETCW (fpcr);
  if (modep == FE_DFL_MODE)
    fpcr_new = (fpcr & _FPU_RESERVED) | _FPU_DEFAULT;
  else
    fpcr_new = *modep;
  if (fpcr != fpcr_new)
    _FPU_SETCW (fpcr_new);
  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  fpu_control_t fpcr;
  fpu_control_t fpcr_new;

  _FPU_GETCW (fpcr);
  excepts &= FE_ALL_EXCEPT;
  fpcr_new = fpcr & ~(excepts << FE_EXCEPT_SHIFT);

  if (fpcr != fpcr_new)
    _FPU_SETCW (fpcr_new);

  return (fpcr >> FE_EXCEPT_SHIFT) & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  fpu_control_t fpcr;
  fpu_control_t fpcr_new;
  fpu_control_t updated_fpcr;

  _FPU_GETCW (fpcr);
  excepts &= FE_ALL_EXCEPT;
  fpcr_new = fpcr | (excepts << FE_EXCEPT_SHIFT);

  if (fpcr != fpcr_new)
    {
      _FPU_SETCW (fpcr_new);

      /* Trapping exceptions are optional in AArch64; the relevant enable
	 bits in FPCR are RES0 hence the absence of support can be detected
	 by reading back the FPCR and comparing with the required value.  */
      _FPU_GETCW (updated_fpcr);

      if (fpcr_new & ~updated_fpcr)
	return -1;
    }

  return (fpcr >> FE_EXCEPT_SHIFT) & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getexcept (void)
{
  fpu_control_t fpcr;
  _FPU_GETCW (fpcr);
  return (fpcr >> FE_EXCEPT_SHIFT) & FE_ALL_EXCEPT;
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
