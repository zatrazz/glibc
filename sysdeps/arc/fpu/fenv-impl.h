/* Architecture-specific implementation of the <fenv.h> functions.
   ARC version.
   Copyright (C) 2020-2026 Free Software Foundation, Inc.
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

#ifndef ARC_FENV_IMPL_H
#define ARC_FENV_IMPL_H 1

#include <fenv.h>
#include <fpu_control.h>

static __always_inline int
fenv_clearexcept (int excepts)
{
  unsigned int fpsr;

  _FPU_GETS (fpsr);

  /* Clear the relevant bits, FWE is preserved.  */
  fpsr &= ~excepts;

  _FPU_SETS (fpsr);

  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  unsigned int fpsr;

  _FPU_GETS (fpsr);
  *flagp = fpsr & excepts;

  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  unsigned int fpsr;

  /* currently raised exceptions are not cleared.  */
  _FPU_GETS (fpsr);
  fpsr |= excepts;

  _FPU_SETS (fpsr);

  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  unsigned int fpsr;

  _FPU_GETS (fpsr);
  fpsr |= excepts;
  _FPU_SETS (fpsr);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  unsigned int fpsr;

  _FPU_GETS (fpsr);

  /* Clear the bits first.  */
  fpsr &= ~excepts;

  /* Now set those bits, copying them over from @flagp.  */
  fpsr |= *flagp & excepts;

  _FPU_SETS (fpsr);

  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  unsigned int fpsr;

  _FPU_GETS (fpsr);

  return fpsr & excepts;
}

static __always_inline int
fenv_getround (void)
{
  unsigned int fpcr;
  _FPU_GETCW (fpcr);

  return (fpcr >> __FPU_RND_SHIFT) & __FPU_RND_MASK;
}

static __always_inline int
fenv_setround (int round)
{
  unsigned int fpcr;

  _FPU_GETCW (fpcr);

  if (((fpcr >> __FPU_RND_SHIFT) & __FPU_RND_MASK) != round)
    {
      fpcr &= ~(__FPU_RND_MASK << __FPU_RND_SHIFT);
      fpcr |= (round & __FPU_RND_MASK) << __FPU_RND_SHIFT;
      _FPU_SETCW (fpcr);
    }

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  unsigned int fpcr;
  unsigned int fpsr;

  _FPU_GETCW (fpcr);
  _FPU_GETS (fpsr);
  envp->__fpcr = fpcr;
  envp->__fpsr = fpsr;

  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  unsigned int fpcr;
  unsigned int fpsr;

  _FPU_GETCW (fpcr);
  _FPU_GETS (fpsr);

  envp->__fpcr = fpcr;
  envp->__fpsr = fpsr;

  fpsr &= ~FE_ALL_EXCEPT;

  _FPU_SETCW (fpcr);
  _FPU_SETS (fpsr);

  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  unsigned int fpcr;
  unsigned int fpsr;

  if (envp == FE_DFL_ENV)
    {
      fpcr = _FPU_DEFAULT;
      fpsr = _FPU_FPSR_DEFAULT;
    }
  else
    {
      /* No need to mask out reserved bits as they are IoW.  */
      fpcr = envp->__fpcr;
      fpsr = envp->__fpsr;
    }

  _FPU_SETCW (fpcr);
  _FPU_SETS (fpsr);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  unsigned int fpcr;
  unsigned int fpsr;

  _FPU_GETS (fpsr);

  if (envp == FE_DFL_ENV)
    {
      fpcr = _FPU_DEFAULT;
    }
  else
    {
      fpcr = envp->__fpcr;

      /* currently raised exceptions need to be preserved.  */
      fpsr |= envp->__fpsr;
    }

  _FPU_SETCW (fpcr);
  _FPU_SETS (fpsr);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  unsigned int fpcr;

  _FPU_GETCW (fpcr);
  *modep = fpcr;

  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  unsigned int fpcr;

  if (modep == FE_DFL_MODE)
    {
      fpcr = _FPU_DEFAULT;
    }
  else
    {
      /* No need to mask out reserved bits as they are IoW.  */
      fpcr = *modep;
    }

  _FPU_SETCW (fpcr);

  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

/* ARC has no support for trapping exceptions.  */

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
