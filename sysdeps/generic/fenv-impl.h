/* Architecture-specific implementation of the <fenv.h> functions.
   Generic version.
   Copyright (C) 2026 Free Software Foundation, Inc.
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

#ifndef _FENV_IMPL_H
#define _FENV_IMPL_H 1

#include <fenv.h>

/* An architecture provides its own <fenv-impl.h>, and the defaults are stubs
   for a floating-point environment without any exception or rounding mode
   support.

   The groups are:

   FENV_IMPL_HAVE_ISO_C: the functions specified by ISO C, that is
   fenv_clearexcept, fenv_getexceptflag, fenv_raiseexcept,
   fenv_setexcept, fenv_setexceptflag, fenv_testexcept, fenv_getround,
   fenv_setround, fenv_getenv, fenv_holdexcept, fenv_setenv,
   fenv_updateenv, fenv_getmode, and fenv_setmode.

   FENV_IMPL_HAVE_TRAP_ENABLE: the GNU extensions to control which
   exceptions trap, that is fenv_disableexcept, fenv_enableexcept, and
   fenv_getexcept.

   Every inline function has the same arguments and return value as the
   <fenv.h> function it implements.

   An architecture may also define FENV_IMPL_HAVE_ENV_OPS and provide the
   primitives the libm internal <fenv_private.h> builds its optimized
   hooks on: fenv_get_env and fenv_set_env read and write the whole
   environment (control and status registers), fenv_update_env (OLD, NEW)
   writes NEW when the current environment is known to be OLD (so that
   unchanged registers can be skipped), fenv_get_control and
   fenv_set_control read and write the control register only, the latter
   keeping the current exception flags, and the fenv_env_* functions
   operate on a saved environment without touching the registers:
   fenv_env_round and fenv_env_set_round for the rounding mode,
   fenv_env_except, fenv_env_set_except (which adds exceptions) and
   fenv_env_clear_except for the exception flags, fenv_env_traps and
   fenv_env_clear_traps for the enabled exception traps.  */

#ifndef FENV_IMPL_HAVE_ISO_C

static __always_inline int
fenv_clearexcept (int excepts)
{
  /* This always fails unless nothing needs to be done.  */
  return (excepts != 0);
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  /* Nothing to do.  */
  *flagp = 0;
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  /* This always fails unless nothing needs to be done.  */
  return (excepts != 0);
}

static __always_inline int
fenv_setexcept (int excepts)
{
  /* This always fails unless nothing needs to be done.  */
  return (excepts != 0);
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  /* This always succeeds, as all exceptions are always clear
     (including in the saved state) so nothing needs to be done.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  return 0;
}

static __always_inline int
fenv_getround (void)
{
#ifdef FE_TONEAREST
  return FE_TONEAREST;
#else
  return 0;
#endif
}

static __always_inline int
fenv_setround (int round)
{
#ifdef FE_TONEAREST
  return (round == FE_TONEAREST) ? 0 : 1;
#else
  return 1;	/* Signal we are unable to set the direction.  */
#endif
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  /* Nothing to do.  */
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  /* No exception traps to disable and no state to save.  */
  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
#if defined FE_NOMASK_ENV && FE_ALL_EXCEPT != 0
  if (envp == FE_NOMASK_ENV)
    return 1;
#endif
  /* Nothing to do.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
#if defined FE_NOMASK_ENV && FE_ALL_EXCEPT != 0
  if (envp == FE_NOMASK_ENV)
    return 1;
#endif
  /* Nothing to do.  */
  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  /* Nothing to do.  */
  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  /* Nothing to do.  */
  return 0;
}

#endif /* !FENV_IMPL_HAVE_ISO_C */

#ifndef FENV_IMPL_HAVE_TRAP_ENABLE

static __always_inline int
fenv_disableexcept (int excepts)
{
  /* All exception traps are disabled.  */
  return 0;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  /* Signal failure if any exception traps are to be enabled.  */
  if (excepts != 0)
    return -1;
  else
    return 0;
}

static __always_inline int
fenv_getexcept (void)
{
  /* All exception traps are disabled.  */
  return 0;
}

#endif /* !FENV_IMPL_HAVE_TRAP_ENABLE */

#endif /* fenv-impl.h */
