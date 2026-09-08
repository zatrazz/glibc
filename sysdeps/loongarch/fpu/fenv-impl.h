/* Architecture-specific implementation of the <fenv.h> functions.
   LoongArch version.
   Copyright (C) 2022-2026 Free Software Foundation, Inc.
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

#ifndef LOONGARCH_FENV_IMPL_H
#define LOONGARCH_FENV_IMPL_H 1

#include <fenv.h>
#include <fenv_libc.h>
#include <fpu_control.h>
#include <get-rounding-mode.h>
#include <float.h>

/* Bits of the FCSR which are status (cause bits and exception flags)
   rather than control modes.  */
#define FCSR_STATUS 0x1f1f0000

#define _FPU_MASK_ALL \
  (_FPU_MASK_V | _FPU_MASK_Z | _FPU_MASK_O | _FPU_MASK_U | _FPU_MASK_I \
   | FE_ALL_EXCEPT)

/* The primitives for the <fenv_private.h> hooks.  The environment is the
   FCSR0 register, with the trap enable bits in the low bits (FE_* shifted
   right by ENABLE_SHIFT), then the rounding mode, the exception flags
   (the FE_* values) and the cause bits (FE_* shifted by CAUSE_SHIFT).
   The rounding mode is also accessible on its own through the FCSR3
   view of the register, which reads as zero elsewhere and whose write
   only affects the rounding mode field; the control primitives use it,
   keeping the rounding mode at its FCSR0 position in the environment.  */

static __always_inline void
fenv_get_env (fenv_t *envp)
{
  _FPU_GETCW (envp->__fp_control_register);
}

static __always_inline void
fenv_set_env (const fenv_t *envp)
{
  fpu_control_t cw __attribute__ ((unused));

  /* Read current state to flush fpu pipeline.  */
  _FPU_GETCW (cw);

  _FPU_SETCW (envp->__fp_control_register);
}

static __always_inline void
fenv_update_env (const fenv_t *old, const fenv_t *new)
{
  _FPU_SETCW (new->__fp_control_register);
}

static __always_inline void
fenv_get_control (fenv_t *envp)
{
  _FPU_GET_RM (envp->__fp_control_register);
}

static __always_inline void
fenv_set_control (const fenv_t *envp)
{
  /* Only the rounding mode field is written, so the current exception
     flags are kept.  */
  _FPU_SET_RM (envp->__fp_control_register);
}

static __always_inline int
fenv_env_round (const fenv_t *envp)
{
  return envp->__fp_control_register & _FPU_RC_MASK;
}

static __always_inline void
fenv_env_set_round (fenv_t *envp, int round)
{
  envp->__fp_control_register
    = (envp->__fp_control_register & ~_FPU_RC_MASK) | round;
}

static __always_inline int
fenv_env_except (const fenv_t *envp)
{
  return envp->__fp_control_register & FE_ALL_EXCEPT;
}

static __always_inline void
fenv_env_set_except (fenv_t *envp, int excepts)
{
  envp->__fp_control_register |= excepts;
}

static __always_inline void
fenv_env_clear_except (fenv_t *envp)
{
  envp->__fp_control_register &= ~FE_ALL_EXCEPT;
}

static __always_inline int
fenv_env_traps (const fenv_t *envp)
{
  return (envp->__fp_control_register & ENABLE_MASK) << ENABLE_SHIFT;
}

static __always_inline void
fenv_env_clear_traps (fenv_t *envp)
{
  envp->__fp_control_register &= ~ENABLE_MASK;
}

#define FENV_IMPL_HAVE_ENV_OPS 1

static __always_inline int
fenv_clearexcept (int excepts)
{
  int cw;

  /* Mask out unsupported bits/exceptions.  */
  excepts &= FE_ALL_EXCEPT;

  /* Read the complete control word.  */
  _FPU_GETCW (cw);

  /* Clear exception flag bits.  */
  cw &= ~excepts;

  /* Put the new data in effect.  */
  _FPU_SETCW (cw);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  fpu_control_t temp;

  /* Get the current exceptions.  */
  _FPU_GETCW (temp);

  /* We only save the relevant bits here.  In particular, care has to be
     taken with the CAUSE bits, as an inadvertent restore later on could
     generate unexpected exceptions.  */

  *flagp = temp & excepts & FE_ALL_EXCEPT;

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  const float fp_zero = 0.0f;
  const float fp_one = 1.0f;
  const float fp_max = FLT_MAX;
  const float fp_min = FLT_MIN;
  const float fp_1e32 = 1.0e32f;
  const float fp_two = 2.0f;
  const float fp_three = 3.0f;

  /* Raise exceptions represented by EXPECTS.  But we must raise only
     one signal at a time.  It is important that if the overflow/underflow
     exception and the inexact exception are given at the same time,
     the overflow/underflow exception follows the inexact exception.  */

  /* First: invalid exception.  */
  if (FE_INVALID & excepts)
    __asm__ __volatile__("fdiv.s $f0,%0,%0\n\t"
			 :
			 : "f"(fp_zero)
			 : "$f0");

  /* Next: division by zero.  */
  if (FE_DIVBYZERO & excepts)
    __asm__ __volatile__("fdiv.s $f0,%0,%1\n\t"
			 :
			 : "f"(fp_one), "f"(fp_zero)
			 : "$f0");

  /* Next: overflow.  */
  if (FE_OVERFLOW & excepts)
    /* There's no way to raise overflow without also raising inexact.  */
    __asm__ __volatile__("fadd.s $f0,%0,%1\n\t"
			 :
			 : "f"(fp_max), "f"(fp_1e32)
			 : "$f0");

  /* Next: underflow.  */
  if (FE_UNDERFLOW & excepts)
    __asm__ __volatile__("fdiv.s $f0,%0,%1\n\t"
			 :
			 : "f"(fp_min), "f"(fp_three)
			 : "$f0");

  /* Last: inexact.  */
  if (FE_INEXACT & excepts)
    __asm__ __volatile__("fdiv.s $f0, %0, %1\n\t"
			 :
			 : "f"(fp_two), "f"(fp_three)
			 : "$f0");

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fpu_control_t temp;

  _FPU_GETCW (temp);
  temp |= excepts & FE_ALL_EXCEPT;
  _FPU_SETCW (temp);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fpu_control_t temp;

  /* Get the current exceptions.  */
  _FPU_GETCW (temp);

  /* Make sure the flags we want restored are legal.  */
  excepts &= FE_ALL_EXCEPT;

  /* Now clear the bits called for, and copy them in from flagp.  Note that
     we ignore all non-flag bits from *flagp, so they don't matter.  */
  temp = (temp & ~excepts) | (*flagp & excepts);

  _FPU_SETCW (temp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  int cw;

  /* Get current control word.  */
  _FPU_GETCW (cw);

  return cw & excepts & FE_ALL_EXCEPT;
}

static __always_inline int
fenv_getround (void)
{
  /* Get RM control word.  */
  return get_rounding_mode ();
}

static __always_inline int
fenv_setround (int round)
{
  if ((round & ~_FPU_RC_MASK) != 0)
    /* ROUND is no valid rounding mode.  */
    return 1;

  /* Set RM state.  */
  _FPU_SET_RM (round);

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  fenv_get_env (envp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  fpu_control_t cw;

  /* Save the current state.  */
  _FPU_GETCW (cw);
  envp->__fp_control_register = cw;

  /* Clear all exception enable bits and flags.  */
  cw &= ~(_FPU_MASK_ALL);
  _FPU_SETCW (cw);

  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  fpu_control_t cw;

  /* Read first current state to flush fpu pipeline.  */
  _FPU_GETCW (cw);

  if (envp == FE_DFL_ENV)
    _FPU_SETCW (_FPU_DEFAULT);
  else if (envp == FE_NOMASK_ENV)
    _FPU_SETCW (_FPU_IEEE);
  else
    _FPU_SETCW (envp->__fp_control_register);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  int temp;

  /* Save current exceptions.  */
  _FPU_GETCW (temp);
  temp &= FE_ALL_EXCEPT;

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
  fpu_control_t cw;

  _FPU_GETCW (cw);
  cw &= FCSR_STATUS;
  if (modep == FE_DFL_MODE)
    cw |= _FPU_DEFAULT;
  else
    cw |= *modep & ~FCSR_STATUS;
  _FPU_SETCW (cw);

  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  unsigned int new_exc, old_exc;

  /* Get the current enables.  */
  _FPU_GET_ENABLES (new_exc);

  old_exc = new_exc << ENABLE_SHIFT;

  excepts &= FE_ALL_EXCEPT;

  new_exc &= ~(excepts >> ENABLE_SHIFT);
  _FPU_SET_ENABLES (new_exc);

  return old_exc;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  unsigned int new_exc, old_exc;

  /* Get the current enables.  */
  _FPU_GET_ENABLES (new_exc);

  old_exc = new_exc << ENABLE_SHIFT;

  excepts &= FE_ALL_EXCEPT;

  new_exc |= excepts >> ENABLE_SHIFT;
  _FPU_SET_ENABLES (new_exc);

  return old_exc;
}

static __always_inline int
fenv_getexcept (void)
{
  unsigned int exc;

  /* Get the current enables.  */
  _FPU_GET_ENABLES (exc);

  return exc << ENABLE_SHIFT;
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
