/* Architecture-specific implementation of the <fenv.h> functions.
   ARM version.
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

#ifndef ARM_FENV_IMPL_H
#define ARM_FENV_IMPL_H 1

#include <fenv.h>
#include <fpu_control.h>
#include <arm-features.h>
#include <get-rounding-mode.h>
#include <float.h>

/* The inline functions used both by the fenv functions and by the libm
   internal <fenv_private.h> hooks.  */

static __always_inline void
libc_feholdexcept_vfp (fenv_t *envp)
{
  fpu_control_t fpscr;

  _FPU_GETCW (fpscr);
  envp->__cw = fpscr;

  /* Clear exception flags and set all exceptions to non-stop.  */
  fpscr &= ~_FPU_MASK_EXCEPT;
  _FPU_SETCW (fpscr);
}

static __always_inline void
libc_fesetround_vfp (int round)
{
  fpu_control_t fpscr;

  _FPU_GETCW (fpscr);

  /* Set new rounding mode if different.  */
  if (__glibc_unlikely ((fpscr & _FPU_MASK_RM) != round))
    _FPU_SETCW ((fpscr & ~_FPU_MASK_RM) | round);
}

static __always_inline int
libc_fetestexcept_vfp (int ex)
{
  fpu_control_t fpscr;

  _FPU_GETCW (fpscr);
  return fpscr & ex & FE_ALL_EXCEPT;
}

static __always_inline void
libc_fesetenv_vfp (const fenv_t *envp)
{
  fpu_control_t fpscr, new_fpscr;

  _FPU_GETCW (fpscr);
  new_fpscr = envp->__cw;

  /* Write new FPSCR if different (ignoring NZCV flags).  */
  if (__glibc_unlikely (((fpscr ^ new_fpscr) & ~_FPU_MASK_NZCV) != 0))
    _FPU_SETCW (new_fpscr);
}

static __always_inline int
libc_feupdateenv_test_vfp (const fenv_t *envp, int ex)
{
  fpu_control_t fpscr, new_fpscr;
  int excepts;

  _FPU_GETCW (fpscr);

  /* Merge current exception flags with the saved fenv.  */
  excepts = fpscr & FE_ALL_EXCEPT;
  new_fpscr = envp->__cw | excepts;

  /* Write new FPSCR if different (ignoring NZCV flags).  */
  if (__glibc_unlikely (((fpscr ^ new_fpscr) & ~_FPU_MASK_NZCV) != 0))
    _FPU_SETCW (new_fpscr);

  /* Raise the exceptions if enabled in the new FP state.  */
  if (__glibc_unlikely (excepts & (new_fpscr >> FE_EXCEPT_SHIFT)))
    __feraiseexcept (excepts);

  return excepts & ex;
}

static __always_inline void
libc_feupdateenv_vfp (const fenv_t *envp)
{
  libc_feupdateenv_test_vfp (envp, 0);
}

/* NZCV flags, QC bit, IDC bit and bits for IEEE exception status.  */
#define FPU_STATUS_BITS 0xf800009f

static __always_inline int
fenv_clearexcept (int excepts)
{
  fpu_control_t fpscr, new_fpscr;

  /* Fail if a VFP unit isn't present unless nothing needs to be done.  */
  if (!ARM_HAVE_VFP)
    return (excepts != 0);

  _FPU_GETCW (fpscr);
  excepts &= FE_ALL_EXCEPT;
  new_fpscr = fpscr & ~excepts;

  /* Write new exception flags if changed.  */
  if (new_fpscr != fpscr)
    _FPU_SETCW (new_fpscr);

  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  /* Fail if a VFP unit isn't present.  */
  if (!ARM_HAVE_VFP)
    return 1;

  *flagp = libc_fetestexcept_vfp (excepts);
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  /* Fail if a VFP unit isn't present unless nothing needs to be done.  */
  if (!ARM_HAVE_VFP)
    return (excepts != 0);
  else
    {
      fpu_control_t fpscr;
      const float fp_zero = 0.0, fp_one = 1.0, fp_max = FLT_MAX,
                  fp_min = FLT_MIN, fp_1e32 = 1.0e32f, fp_two = 2.0,
		  fp_three = 3.0;

      /* Raise exceptions represented by EXPECTS.  But we must raise only
	 one signal at a time.  It is important that if the overflow/underflow
	 exception and the inexact exception are given at the same time,
	 the overflow/underflow exception follows the inexact exception.  After
	 each exception we read from the fpscr, to force the exception to be
	 raised immediately.  */

      /* There are additional complications because this file may be compiled
         without VFP support enabled, and we also can't assume that the
	 assembler has VFP instructions enabled. To get around this we use the
	 generic coprocessor mnemonics and avoid asking GCC to put float values
	 in VFP registers.  */

      /* First: invalid exception.  */
      if (FE_INVALID & excepts)
	__asm__ __volatile__ (
	  "ldc p10, cr0, %1\n\t"                        /* flds s0, %1  */
	  "cdp p10, 8, cr0, cr0, cr0, 0\n\t"            /* fdivs s0, s0, s0  */
	  "mrc p10, 7, %0, cr1, cr0, 0" : "=r" (fpscr)  /* fmrx %0, fpscr  */
			                : "m" (fp_zero)
					: "s0");

      /* Next: division by zero.  */
      if (FE_DIVBYZERO & excepts)
	__asm__ __volatile__ (
	  "ldc p10, cr0, %1\n\t"                        /* flds s0, %1  */
	  "ldcl p10, cr0, %2\n\t"                       /* flds s1, %2  */
	  "cdp p10, 8, cr0, cr0, cr0, 1\n\t"            /* fdivs s0, s0, s1  */
	  "mrc p10, 7, %0, cr1, cr0, 0" : "=r" (fpscr)  /* fmrx %0, fpscr  */
			                : "m" (fp_one), "m" (fp_zero)
					: "s0", "s1");

      /* Next: overflow.  */
      if (FE_OVERFLOW & excepts)
	/* There's no way to raise overflow without also raising inexact.  */
	__asm__ __volatile__ (
	  "ldc p10, cr0, %1\n\t"                        /* flds s0, %1  */
	  "ldcl p10, cr0, %2\n\t"                       /* flds s1, %2  */
	  "cdp p10, 3, cr0, cr0, cr0, 1\n\t"            /* fadds s0, s0, s1  */
	  "mrc p10, 7, %0, cr1, cr0, 0" : "=r" (fpscr)  /* fmrx %0, fpscr  */
			                : "m" (fp_max), "m" (fp_1e32)
					: "s0", "s1");

      /* Next: underflow.  */
      if (FE_UNDERFLOW & excepts)
	__asm__ __volatile__ (
	  "ldc p10, cr0, %1\n\t"                        /* flds s0, %1  */
	  "ldcl p10, cr0, %2\n\t"                       /* flds s1, %2  */
	  "cdp p10, 8, cr0, cr0, cr0, 1\n\t"            /* fdivs s0, s0, s1  */
	  "mrc p10, 7, %0, cr1, cr0, 0" : "=r" (fpscr)  /* fmrx %0, fpscr  */
			                : "m" (fp_min), "m" (fp_three)
					: "s0", "s1");

      /* Last: inexact.  */
      if (FE_INEXACT & excepts)
	__asm__ __volatile__ (
	  "ldc p10, cr0, %1\n\t"                        /* flds s0, %1  */
	  "ldcl p10, cr0, %2\n\t"                       /* flds s1, %2  */
	  "cdp p10, 8, cr0, cr0, cr0, 1\n\t"            /* fdivs s0, s0, s1  */
	  "mrc p10, 7, %0, cr1, cr0, 0" : "=r" (fpscr)  /* fmrx %0, fpscr  */
			                : "m" (fp_two), "m" (fp_three)
					: "s0", "s1");

      /* Success.  */
      return 0;
    }
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fpu_control_t fpscr, new_fpscr;

  /* Fail if a VFP unit isn't present unless nothing needs to be done.  */
  if (!ARM_HAVE_VFP)
    return (excepts != 0);

  _FPU_GETCW (fpscr);
  new_fpscr = fpscr | (excepts & FE_ALL_EXCEPT);
  if (new_fpscr != fpscr)
    _FPU_SETCW (new_fpscr);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fpu_control_t fpscr, new_fpscr;

  /* Fail if a VFP unit isn't present unless nothing needs to be done.  */
  if (!ARM_HAVE_VFP)
    return (excepts != 0);

  _FPU_GETCW (fpscr);
  excepts &= FE_ALL_EXCEPT;

  /* Set the desired exception mask.  */
  new_fpscr = fpscr & ~excepts;
  new_fpscr |= *flagp & excepts;

  /* Write new exception flags if changed.  */
  if (new_fpscr != fpscr)
    _FPU_SETCW (new_fpscr);

  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  /* Return no exception flags if a VFP unit isn't present.  */
  if (!ARM_HAVE_VFP)
    return 0;

  return libc_fetestexcept_vfp (excepts);
}

static __always_inline int
fenv_getround (void)
{
  return get_rounding_mode ();
}

static __always_inline int
fenv_setround (int round)
{
  /* FE_TONEAREST is the only supported rounding mode
     if a VFP unit isn't present.  */
  if (!ARM_HAVE_VFP)
    return (round == FE_TONEAREST) ? 0 : 1;

  if (round & ~_FPU_MASK_RM)
    return 1;

  libc_fesetround_vfp (round);
  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  fpu_control_t fpscr;

  /* Fail if a VFP unit isn't present.  */
  if (!ARM_HAVE_VFP)
    return 1;

  _FPU_GETCW (fpscr);
  envp->__cw = fpscr;
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  /* Fail if a VFP unit isn't present.  */
  if (!ARM_HAVE_VFP)
    return 1;

  libc_feholdexcept_vfp (envp);
  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  fpu_control_t fpscr, new_fpscr, updated_fpscr;

  /* Fail if a VFP unit isn't present.  */
  if (!ARM_HAVE_VFP)
    return 1;

  if ((envp != FE_DFL_ENV) && (envp != FE_NOMASK_ENV))
    {
      /* The new FPSCR is valid, so don't merge the reserved flags.  */
      libc_fesetenv_vfp (envp);
      return 0;
    }

  _FPU_GETCW (fpscr);

  /* Preserve the reserved FPSCR flags.  */
  new_fpscr = fpscr & _FPU_RESERVED;
  new_fpscr |= (envp == FE_DFL_ENV) ? _FPU_DEFAULT : _FPU_IEEE;

  if (((new_fpscr ^ fpscr) & ~_FPU_MASK_NZCV) != 0)
    {
      _FPU_SETCW (new_fpscr);

      /* Not all VFP architectures support trapping exceptions, so
	 test whether the relevant bits were set and fail if not.  */
      _FPU_GETCW (updated_fpscr);

      return new_fpscr & ~updated_fpscr;
    }

  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  fpu_control_t fpscr, new_fpscr, updated_fpscr;
  int excepts;

  /* Fail if a VFP unit isn't present.  */
  if (!ARM_HAVE_VFP)
    return 1;

  if ((envp != FE_DFL_ENV) && (envp != FE_NOMASK_ENV))
    {
      /* Merge current exception flags with the saved fenv, and raise
	 the exceptions if enabled in the new FP state.  */
      libc_feupdateenv_vfp (envp);
      return 0;
    }

  _FPU_GETCW (fpscr);
  excepts = fpscr & FE_ALL_EXCEPT;

  /* Preserve the reserved FPSCR flags.  */
  new_fpscr = fpscr & (_FPU_RESERVED | FE_ALL_EXCEPT);
  new_fpscr |= (envp == FE_DFL_ENV) ? _FPU_DEFAULT : _FPU_IEEE;

  if (((new_fpscr ^ fpscr) & ~_FPU_MASK_NZCV) != 0)
    {
      _FPU_SETCW (new_fpscr);

      /* Not all VFP architectures support trapping exceptions, so
	 test whether the relevant bits were set and fail if not.  */
      _FPU_GETCW (updated_fpscr);

      if (new_fpscr & ~updated_fpscr)
	return 1;
    }

  /* Raise the exceptions if enabled in the new FP state.  */
  if (excepts & (new_fpscr >> FE_EXCEPT_SHIFT))
    return __feraiseexcept (excepts);

  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  if (ARM_HAVE_VFP)
    _FPU_GETCW (*modep);
  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  fpu_control_t fpscr, new_fpscr;

  if (!ARM_HAVE_VFP)
    /* Nothing to do.  */
    return 0;

  _FPU_GETCW (fpscr);
  if (modep == FE_DFL_MODE)
    new_fpscr = (fpscr & (_FPU_RESERVED | FPU_STATUS_BITS)) | _FPU_DEFAULT;
  else
    new_fpscr = (fpscr & FPU_STATUS_BITS) | (*modep & ~FPU_STATUS_BITS);

  if (((new_fpscr ^ fpscr) & ~_FPU_MASK_NZCV) != 0)
    _FPU_SETCW (new_fpscr);

  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  fpu_control_t fpscr, new_fpscr;

  /* Fail if a VFP unit isn't present.  */
  if (!ARM_HAVE_VFP)
    return -1;

  _FPU_GETCW (fpscr);
  excepts &= FE_ALL_EXCEPT;
  new_fpscr = fpscr & ~(excepts << FE_EXCEPT_SHIFT);

  /* Write new exceptions if changed.  */
  if (new_fpscr != fpscr)
    _FPU_SETCW (new_fpscr);

  return (fpscr >> FE_EXCEPT_SHIFT) & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  fpu_control_t fpscr, new_fpscr, updated_fpscr;

  /* Fail if a VFP unit isn't present.  */
  if (!ARM_HAVE_VFP)
    return -1;

  _FPU_GETCW (fpscr);
  excepts &= FE_ALL_EXCEPT;
  new_fpscr = fpscr | (excepts << FE_EXCEPT_SHIFT);

  if (new_fpscr != fpscr)
    {
      _FPU_SETCW (new_fpscr);

      /* Not all VFP architectures support trapping exceptions, so
	 test whether the relevant bits were set and fail if not.  */
      _FPU_GETCW (updated_fpscr);

      if (new_fpscr & ~updated_fpscr)
	return -1;
    }

  return (fpscr >> FE_EXCEPT_SHIFT) & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getexcept (void)
{
  fpu_control_t fpscr;

  /* Return with all exceptions disabled if a VFP unit isn't present.  */
  if (!ARM_HAVE_VFP)
    return 0;

  _FPU_GETCW (fpscr);

  return (fpscr >> FE_EXCEPT_SHIFT) & FE_ALL_EXCEPT;
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
