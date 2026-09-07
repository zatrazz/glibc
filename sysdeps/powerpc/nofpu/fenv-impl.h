/* Architecture-specific implementation of the <fenv.h> functions.
   PowerPC soft-float version.
   Copyright (C) 2002-2026 Free Software Foundation, Inc.
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

#ifndef POWERPC_NOFPU_FENV_IMPL_H
#define POWERPC_NOFPU_FENV_IMPL_H 1

#include <fenv.h>
#include <soft-fp.h>
#include <soft-supp.h>
#include <signal.h>

static __always_inline int
fenv_clearexcept (int excepts)
{
  __sim_exceptions_thread &= ~excepts;
  SIM_SET_GLOBAL (__sim_exceptions_global, __sim_exceptions_thread);
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  *flagp = (fexcept_t) __sim_exceptions_thread & excepts & FE_ALL_EXCEPT;

  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  __sim_exceptions_thread |= excepts;
  SIM_SET_GLOBAL (__sim_exceptions_global, __sim_exceptions_thread);
  if (excepts & ~__sim_disabled_exceptions_thread)
    raise (SIGFPE);
  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  __sim_exceptions_thread |= (excepts & FE_ALL_EXCEPT);
  SIM_SET_GLOBAL (__sim_exceptions_global, __sim_exceptions_thread);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  /* Ignore exceptions not listed in 'excepts'.  */
  __sim_exceptions_thread
    = (__sim_exceptions_thread & ~excepts) | (*flagp & excepts);
  SIM_SET_GLOBAL (__sim_exceptions_global, __sim_exceptions_thread);

  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  return __sim_exceptions_thread & excepts;
}

static __always_inline int
fenv_getround (void)
{
  return __sim_round_mode_thread;
}

static __always_inline int
fenv_setround (int round)
{
  if ((unsigned int) round > FE_DOWNWARD)
    return 1;

  __sim_round_mode_thread = round;
  SIM_SET_GLOBAL (__sim_round_mode_global, __sim_round_mode_thread);

  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  fenv_union_t u;

  u.l[0] = __sim_exceptions_thread;
  u.l[0] |= __sim_round_mode_thread;
  u.l[1] = __sim_disabled_exceptions_thread;

  *envp = u.fenv;

  return 0;
}

/* Defined below; used by fenv_holdexcept.  */
static __always_inline int fenv_setenv (const fenv_t *envp);

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  fenv_union_t u;

  /* Get the current state.  */
  fenv_getenv (envp);

  u.fenv = *envp;
  /* Clear everything except the rounding mode.  */
  u.l[0] &= 0x3;
  /* Disable exceptions */
  u.l[1] = FE_ALL_EXCEPT;

  /* Put the new state in effect.  */
  fenv_setenv (&u.fenv);

  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  fenv_union_t u;

  u.fenv = *envp;
  __sim_exceptions_thread = u.l[0] & FE_ALL_EXCEPT;
  SIM_SET_GLOBAL (__sim_exceptions_global, __sim_exceptions_thread);
  __sim_round_mode_thread = u.l[0] & 0x3;
  SIM_SET_GLOBAL (__sim_round_mode_global, __sim_round_mode_thread);
  __sim_disabled_exceptions_thread = u.l[1];
  SIM_SET_GLOBAL (__sim_disabled_exceptions_global,
		  __sim_disabled_exceptions_thread);
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  int saved_exceptions;

  /* Save currently set exceptions.  */
  saved_exceptions = __sim_exceptions_thread;

  /* Set environment.  */
  fenv_setenv (envp);

  /* Raise old exceptions.  */
  __sim_exceptions_thread |= saved_exceptions;
  SIM_SET_GLOBAL (__sim_exceptions_global, __sim_exceptions_thread);
  if (saved_exceptions & ~__sim_disabled_exceptions_thread)
    raise (SIGFPE);

  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  fenv_union_t u;

  u.l[0] = __sim_round_mode_thread;
  u.l[1] = __sim_disabled_exceptions_thread;

  *modep = u.fenv;

  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  fenv_union_t u;

  u.fenv = *modep;
  __sim_round_mode_thread = u.l[0];
  SIM_SET_GLOBAL (__sim_round_mode_global, __sim_round_mode_thread);
  __sim_disabled_exceptions_thread = u.l[1];
  SIM_SET_GLOBAL (__sim_disabled_exceptions_global,
		  __sim_disabled_exceptions_thread);
  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  int old_exceptions = ~__sim_disabled_exceptions_thread & FE_ALL_EXCEPT;

  __sim_disabled_exceptions_thread |= excepts;
  SIM_SET_GLOBAL (__sim_disabled_exceptions_global,
		  __sim_disabled_exceptions_thread);

  return old_exceptions;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  int old_exceptions = ~__sim_disabled_exceptions_thread & FE_ALL_EXCEPT;

  __sim_disabled_exceptions_thread &= ~excepts;
  SIM_SET_GLOBAL (__sim_disabled_exceptions_global,
		  __sim_disabled_exceptions_thread);

  return old_exceptions;
}

static __always_inline int
fenv_getexcept (void)
{
  return (__sim_disabled_exceptions_thread ^ FE_ALL_EXCEPT) & FE_ALL_EXCEPT;
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
