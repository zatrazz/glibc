/* Architecture-specific implementation of the <fenv.h> functions.
   MIPS version.
   Copyright (C) 1998-2026 Free Software Foundation, Inc.
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

#ifndef MIPS_FENV_IMPL_H
#define MIPS_FENV_IMPL_H 1

#include <fenv.h>
#include <fenv_libc.h>
#include <fpu_control.h>

/* Bits of the FCSR which are status (condition codes, cause bits and
   exception flags) rather than control modes.  */
#define FCSR_STATUS 0xfe83f07c

#define _FPU_MASK_ALL (_FPU_MASK_V | _FPU_MASK_Z | _FPU_MASK_O \
		       |_FPU_MASK_U | _FPU_MASK_I | FE_ALL_EXCEPT)

/* The inline functions used both by the fenv functions and by the libm
   internal <fenv_private.h> hooks.  */

static __always_inline void
libc_fesetround_mips (int round)
{
  fpu_control_t cw;

  /* Get current state.  */
  _FPU_GETCW (cw);

  /* Set rounding bits.  */
  cw &= ~_FPU_RC_MASK;
  cw |= round;

  /* Set new state.  */
  _FPU_SETCW (cw);
}

static __always_inline int
fenv_clearexcept (int excepts)
{
  int cw;

  /* Mask out unsupported bits/exceptions.  */
  excepts &= FE_ALL_EXCEPT;

  /* Read the complete control word.  */
  _FPU_GETCW (cw);

  /* Clear exception flag bits and cause bits. If the cause bit is not
     cleared, the next CTC instruction (just below) will re-generate the
     exception.  */

  cw &= ~(excepts | (excepts << CAUSE_SHIFT));

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
  fpu_control_t cw;

  /* Get current state.  */
  _FPU_GETCW (cw);

  /* Set flag bits (which are accumulative), and *also* set the
     cause bits. The setting of the cause bits is what actually causes
     the hardware to generate the exception, if the corresponding enable
     bit is set as well.  */

  excepts &= FE_ALL_EXCEPT;
  cw |= excepts | (excepts << CAUSE_SHIFT);

  /* Set new state.  */
  _FPU_SETCW (cw);

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

  /* Now clear the bits called for, and copy them in from flagp. Note that
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
  int cw;

  /* Get control word.  */
  _FPU_GETCW (cw);

  return cw & _FPU_RC_MASK;
}

static __always_inline int
fenv_setround (int round)
{
  if ((round & ~_FPU_RC_MASK) != 0)
    /* ROUND is no valid rounding mode.  */
    return 1;

  libc_fesetround_mips (round);

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  _FPU_GETCW (*envp);

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

  /* Get the current control word.  */
  _FPU_GETCW (new_exc);

  old_exc = (new_exc & ENABLE_MASK) >> ENABLE_SHIFT;

  excepts &= FE_ALL_EXCEPT;

  new_exc &= ~(excepts << ENABLE_SHIFT);
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
