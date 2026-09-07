/* Architecture-specific implementation of the <fenv.h> functions.
   PowerPC version.
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

#ifndef POWERPC_FENV_IMPL_H
#define POWERPC_FENV_IMPL_H 1

#include <fenv.h>
#include <fenv_libc.h>

static __always_inline int
fenv_clearexcept (int excepts)
{
  fenv_union_t u, n;

  /* Get the current state.  */
  u.fenv = fegetenv_register ();

  /* Clear the relevant bits.  */
  n.l = u.l & ~((-(excepts >> (31 - FPSCR_VX) & 1) & FE_ALL_INVALID)
		| (excepts & FPSCR_STICKY_BITS));

  /* Put the new state in effect.  */
  if (u.l != n.l)
    fesetenv_register (n.fenv);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  fenv_union_t u;

  /* Get the current state.  */
  u.fenv = fegetenv_register ();

  /* Return (all of) it.  */
  *flagp = u.l & excepts & FE_ALL_EXCEPT;

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  fenv_union_t u;

  /* Raise exceptions represented by EXCEPTS.  It is the responsibility of
     the OS to ensure that if multiple exceptions occur they are fed back
     to this process in the proper way; this can happen in hardware,
     anyway (in particular, inexact with overflow or underflow). */

  /* Get the current state.  */
  u.fenv = fegetenv_register ();

  /* Add the exceptions */
  u.l = (u.l
	 | (excepts & FPSCR_STICKY_BITS)
	 /* Turn FE_INVALID into FE_INVALID_SOFTWARE.  */
	 | (excepts >> ((31 - FPSCR_VX) - (31 - FPSCR_VXSOFT))
	    & FE_INVALID_SOFTWARE));

  /* Store the new status word (along with the rest of the environment),
     triggering any appropriate exceptions.  */
  fesetenv_register (u.fenv);

  if ((excepts & FE_INVALID))
    {
      /* For some reason, some PowerPC chips (the 601, in particular)
	 don't have FE_INVALID_SOFTWARE implemented.  Detect this
	 case and raise FE_INVALID_SNAN instead.  */
      u.fenv = fegetenv_register ();
      if ((u.l & FE_INVALID) == 0)
	set_fpscr_bit (FPSCR_VXSNAN);
    }

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fenv_union_t u, n;

  u.fenv = fegetenv_register ();
  n.l = (u.l
	 | (excepts & FPSCR_STICKY_BITS)
	 /* Turn FE_INVALID into FE_INVALID_SOFTWARE.  */
	 | (excepts >> ((31 - FPSCR_VX) - (31 - FPSCR_VXSOFT))
	    & FE_INVALID_SOFTWARE));
  if (n.l != u.l)
    {
      if (n.l & fenv_exceptions_to_reg (excepts))
	/* Setting the exception flags may trigger a trap.  ISO C 23 § 7.6.4.4
	    does not allow it.   */
	return -1;

      fesetenv_register (n.fenv);

      /* Deal with FE_INVALID_SOFTWARE not being implemented on some chips.  */
      if (excepts & FE_INVALID)
	__feraiseexcept (FE_INVALID);
    }

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fenv_union_t u, n;
  fexcept_t flag;

  /* Get the current state.  */
  u.fenv = fegetenv_register ();

  /* Ignore exceptions not listed in 'excepts'.  */
  flag = *flagp & excepts;

  /* Replace the exception status */
  int excepts_mask = FPSCR_STICKY_BITS & excepts;
  if ((excepts & FE_INVALID) != 0)
    excepts_mask |= FE_ALL_INVALID;
  n.l = ((u.l & ~excepts_mask)
	 | (flag & FPSCR_STICKY_BITS)
	 /* Turn FE_INVALID into FE_INVALID_SOFTWARE.  */
	 | (flag >> ((31 - FPSCR_VX) - (31 - FPSCR_VXSOFT))
	    & FE_INVALID_SOFTWARE));

  /* Store the new status word (along with the rest of the environment).
     This may cause floating-point exceptions if the restored state
     requests it.  */
  if (n.l != u.l)
    {
      if (n.l & fenv_exceptions_to_reg (excepts))
	/* Setting the exception flags may trigger a trap.  ISO C 23 § 7.6.4.4
	    does not allow it.   */
	return -1;

      fesetenv_register (n.fenv);
    }

  /* Deal with FE_INVALID_SOFTWARE not being implemented on some chips.  */
  if (flag & FE_INVALID)
    __feraiseexcept (FE_INVALID);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  fenv_union_t u;

  /* Get the current state.  */
  u.fenv = fegetenv_register ();

  /* The FE_INVALID bit is dealt with correctly by the hardware, so we can
     just:  */
  return u.l & excepts;
}

static __always_inline int
fenv_getround (void)
{
  fenv_union_t fe;

  fe.fenv = fegetenv_control ();

  return fe.l & 0x3;
}

static __always_inline int
fenv_setround (int round)
{
  if ((unsigned int) round > 3)
    return 1;
  else
    return __fesetround_inline (round);
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  *envp = fegetenv_register ();

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  fenv_union_t old, new;

  /* Save the currently set exceptions.  */
  old.fenv = *envp = fegetenv_register ();

  /* Clear everything except for the rounding modes and non-IEEE arithmetic
     flag.  */
  new.l = old.l & 0xffffffff00000007LL;

  if (new.l == old.l)
    return 0;

  __TEST_AND_ENTER_NON_STOP (old.l, 0ULL);

  /* Put the new state in effect.  */
  fesetenv_register (new.fenv);

  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  fenv_union_t old, new;

  /* get the currently set exceptions.  */
  new.fenv = *envp;
  old.fenv = fegetenv_control ();

  __TEST_AND_EXIT_NON_STOP (old.l, new.l);
  __TEST_AND_ENTER_NON_STOP (old.l, new.l);

  fesetenv_register (new.fenv);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  fenv_union_t old, new;

  /* Save the currently set exceptions.  */
  new.fenv = *envp;
  old.fenv = fegetenv_register ();

  /* Restore rounding mode and exception enable from *envp and merge
     exceptions.  Leave fraction rounded/inexact and FP result/CC bits
     unchanged.  */
  new.l = (old.l & 0xffffffff1fffff00LL) | (new.l & 0x1ff80fff);

  __TEST_AND_EXIT_NON_STOP (old.l, new.l);
  __TEST_AND_ENTER_NON_STOP (old.l, new.l);

  /* Atomically enable and raise (if appropriate) exceptions set in `new'. */
  fesetenv_register (new.fenv);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getmode (femode_t *modep)
{
  *modep = fegetenv_control ();
  return 0;
}

static __always_inline int
fenv_setmode (const femode_t *modep)
{
  fenv_union_t old, new;

  /* Logic regarding enabled exceptions as in fesetenv.  */

  new.fenv = *modep;
  old.fenv = fegetenv_control ();
  new.l = (new.l & ~FPSCR_STATUS_MASK) | (old.l & FPSCR_STATUS_MASK);

  if (old.l == new.l)
    return 0;

  __TEST_AND_EXIT_NON_STOP (old.l, new.l);
  __TEST_AND_ENTER_NON_STOP (old.l, new.l);

  fesetenv_control (new.fenv);
  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  fenv_union_t fe, curr;
  int result, new;

  /* Get current exception mask to return.  */
  fe.fenv = curr.fenv = fegetenv_control ();
  result = fenv_reg_to_exceptions (fe.l);

  if ((excepts & FE_ALL_INVALID) == FE_ALL_INVALID)
    excepts = (excepts | FE_INVALID) & ~ FE_ALL_INVALID;

  new = fenv_exceptions_to_reg (excepts);

  if (fenv_reg_to_exceptions (new) != excepts)
    return -1;

  /* Sets the new exception mask.  */
  fe.l &= ~new;

  if (fe.l != curr.l)
    fesetenv_control (fe.fenv);

  __TEST_AND_ENTER_NON_STOP (-1ULL, fe.l);

  return result;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  fenv_union_t fe, curr;
  int result, new;

  /* Get current exception mask to return.  */
  fe.fenv = curr.fenv = fegetenv_control ();
  result = fenv_reg_to_exceptions (fe.l);

  if ((excepts & FE_ALL_INVALID) == FE_ALL_INVALID)
    excepts = (excepts | FE_INVALID) & ~ FE_ALL_INVALID;

  new = fenv_exceptions_to_reg (excepts);

  if (fenv_reg_to_exceptions (new) != excepts)
    return -1;

  /* Sets the new exception mask.  */
  fe.l |= new;

  if (fe.l != curr.l)
    fesetenv_control (fe.fenv);

  __TEST_AND_EXIT_NON_STOP (0ULL, fe.l);

  return result;
}

static __always_inline int
fenv_getexcept (void)
{
  fenv_union_t fe;

  fe.fenv = fegetenv_control ();

  return fenv_reg_to_exceptions (fe.l);
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
