/* Architecture-specific implementation of the <fenv.h> functions.
   S/390 version.
   Copyright (C) 2000-2026 Free Software Foundation, Inc.
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

#ifndef S390_FENV_IMPL_H
#define S390_FENV_IMPL_H 1

#include <fenv.h>
#include <fenv_libc.h>
#include <fpu_control.h>
#include <get-rounding-mode.h>
#include <float.h>

/* The inline functions used both by the fenv functions and by the libm
   internal <fenv_private.h> hooks.  */

static __always_inline void
libc_fesetround_s390 (int round)
{
  __asm__ __volatile__ ("srnm 0(%0)" : : "a" (round));
}

/* The status part of the FPC: the exception flags and the DXC.  */
#define FPC_STATUS (FPC_FLAGS_MASK | FPC_DXC_MASK)

/* The primitives for the <fenv_private.h> hooks.  The environment is the
   FPC register: the exception trap masks (bits 0-7), the exception flags
   (bits 8-15), the data exception code (bits 16-23) and the rounding mode
   (bits 29-31), see <fenv_libc.h>.  */

static __always_inline void
fenv_get_env (fenv_t *envp)
{
  _FPU_GETCW (envp->__fpc);
}

static __always_inline void
fenv_set_env (const fenv_t *envp)
{
  _FPU_SETCW (envp->__fpc);
}

static __always_inline void
fenv_update_env (const fenv_t *old, const fenv_t *new)
{
  if (new->__fpc != old->__fpc)
    _FPU_SETCW (new->__fpc);
}

static __always_inline void
fenv_get_control (fenv_t *envp)
{
  _FPU_GETCW (envp->__fpc);
}

static __always_inline void
fenv_set_control (const fenv_t *envp)
{
  fpu_control_t fpc;

  /* Keep the current exception flags.  */
  _FPU_GETCW (fpc);
  fpc = envp->__fpc | (fpc & FPC_FLAGS_MASK);
  _FPU_SETCW (fpc);
}

static __always_inline int
fenv_env_round (const fenv_t *envp)
{
  return envp->__fpc & FPC_RM_MASK;
}

static __always_inline void
fenv_env_set_round (fenv_t *envp, int round)
{
  envp->__fpc = (envp->__fpc & ~FPC_RM_MASK) | (round & FPC_RM_MASK);
}

static __always_inline int
fenv_env_except (const fenv_t *envp)
{
  int excepts = (envp->__fpc >> FPC_FLAGS_SHIFT) & FE_ALL_EXCEPT;
  if ((envp->__fpc & FPC_NOT_FPU_EXCEPTION) == 0)
    /* Bits 6, 7 of dxc-byte are zero,
       thus bits 0-5 of dxc-byte correspond to the flag-bits.
       Evaluate flags and last dxc-exception-code.  */
    excepts |= (envp->__fpc >> FPC_DXC_SHIFT) & FE_ALL_EXCEPT;
  return excepts;
}

static __always_inline void
fenv_env_set_except (fenv_t *envp, int excepts)
{
  envp->__fpc |= excepts << FPC_FLAGS_SHIFT;
}

static __always_inline void
fenv_env_clear_except (fenv_t *envp)
{
  /* Clear the exception flags and the dxc field.  */
  envp->__fpc &= ~FPC_STATUS;
}

static __always_inline int
fenv_env_traps (const fenv_t *envp)
{
  return (envp->__fpc >> FPC_EXCEPTION_MASK_SHIFT) & FE_ALL_EXCEPT;
}

static __always_inline void
fenv_env_clear_traps (fenv_t *envp)
{
  envp->__fpc &= ~FPC_EXCEPTION_MASK;
}

#define FENV_IMPL_HAVE_ENV_OPS 1

/* Install ENVP and raise the exceptions raised since it was saved, as
   feupdateenv does for a user environment.  */
static __always_inline void
s390_feupdateenv (const fenv_t *envp)
{
  fenv_t cur, new;
  int excepts;

  fenv_get_env (&cur);
  excepts = fenv_env_except (&cur);

  /* Merge the currently raised exceptions with those in envp.  */
  new = *envp;
  fenv_env_set_except (&new, excepts);
  fenv_update_env (&cur, &new);

  /* Raise the exceptions if enabled in new fpc.  */
  if (__glibc_unlikely (excepts & fenv_env_traps (&new)))
    __feraiseexcept (excepts);
}

static __always_inline fenv_t
libc_handle_user_fenv_s390 (const fenv_t *envp)
{
  fenv_t env;
  if (envp == FE_DFL_ENV)
    {
      env.__fpc = _FPU_DEFAULT;
    }
  else if (envp == FE_NOMASK_ENV)
    {
      env.__fpc = FPC_EXCEPTION_MASK;
    }
  else
    env = (*envp);

  return env;
}

static __always_inline void
fexceptdiv (float d, float e)
{
  __asm__ __volatile__ ("debr %0,%1" : : "f" (d), "f" (e) );
}

static __always_inline void
fexceptadd (float d, float e)
{
  __asm__ __volatile__ ("aebr %0,%1" : : "f" (d), "f" (e) );
}

#ifdef HAVE_S390_MIN_Z196_ZARCH_ASM_SUPPORT
static __always_inline void
fexceptround (double e)
{
  float d;
  /* Load rounded from double to float with M3 = round toward 0, M4 = Suppress
     IEEE-inexact exception.
     In case of e=0x1p128 and the overflow-mask bit is zero, only the
     IEEE-overflow flag is set. If overflow-mask bit is one, DXC field is set to
     0x20 "IEEE overflow, exact".
     In case of e=0x1p-150 and the underflow-mask bit is zero, only the
     IEEE-underflow flag is set. If underflow-mask bit is one, DXC field is set
     to 0x10 "IEEE underflow, exact".
     This instruction is available with a zarch machine >= z196.  */
  __asm__ __volatile__ ("ledbra %0,5,%1,4" : "=f" (d) : "f" (e) );
}
#endif

static __always_inline int
fenv_clearexcept (int excepts)
{
  fexcept_t temp;

  /* Mask out unsupported bits/exceptions.  */
  excepts &= FE_ALL_EXCEPT;

  _FPU_GETCW (temp);
  /* Clear the relevant bits.  */
  temp &= ~(excepts << FPC_FLAGS_SHIFT);
  if ((temp & FPC_NOT_FPU_EXCEPTION) == 0)
    /* Bits 6, 7 of dxc-byte are zero,
       thus bits 0-5 of dxc-byte correspond to the flag-bits.
       Clear the relevant bits in flags and dxc-field.  */
    temp &= ~(excepts << FPC_DXC_SHIFT);

  /* Put the new data in effect.  */
  _FPU_SETCW (temp);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_getexceptflag (fexcept_t *flagp, int excepts)
{
  int res;
  fexcept_t fpc;

  _FPU_GETCW (fpc);

  /* Get current exceptions.  */
  res = (fpc >> FPC_FLAGS_SHIFT) & FE_ALL_EXCEPT;
  if ((fpc & FPC_NOT_FPU_EXCEPTION) == 0)
    /* Bits 6, 7 of dxc-byte are zero,
       thus bits 0-5 of dxc-byte correspond to the flag-bits.
       Evaluate flags and last dxc-exception-code.  */
    res |= (fpc >> FPC_DXC_SHIFT) & FE_ALL_EXCEPT;

  *flagp = res & excepts;

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_raiseexcept (int excepts)
{
  /* Raise exceptions represented by EXPECTS.  But we must raise only
     one signal at a time.  It is important that if the overflow/underflow
     exception and the inexact exception are given at the same time,
     the overflow/underflow exception follows the inexact exception.  */

  /* First: invalid exception.  */
  if (FE_INVALID & excepts)
    fexceptdiv (0.0, 0.0);

  /* Next: division by zero.  */
  if (FE_DIVBYZERO & excepts)
    fexceptdiv (1.0, 0.0);

  /* Next: overflow.  */
  if (FE_OVERFLOW & excepts)
    {
#ifdef HAVE_S390_MIN_Z196_ZARCH_ASM_SUPPORT
      fexceptround (0x1p128);
#else
      /* If overflow-mask bit is zero, both IEEE-overflow and IEEE-inexact flags
	 are set.  If overflow-mask bit is one, DXC field is set to 0x2C "IEEE
	 overflow, inexact and incremented".  */
      fexceptadd (FLT_MAX, 1.0e32);
#endif
    }

  /* Next: underflow.  */
  if (FE_UNDERFLOW & excepts)
    {
#ifdef HAVE_S390_MIN_Z196_ZARCH_ASM_SUPPORT
      fexceptround (0x1p-150);
#else
      /* If underflow-mask bit is zero, both IEEE-underflow and IEEE-inexact
	 flags are set.  If underflow-mask bit is one, DXC field is set to 0x1C
	 "IEEE underflow, inexact and incremented".  */
      fexceptdiv (FLT_MIN, 3.0);
#endif
    }

  /* Last: inexact.  */
  if (FE_INEXACT & excepts)
    fexceptdiv (2.0, 3.0);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_setexcept (int excepts)
{
  fexcept_t temp;

  _FPU_GETCW (temp);
  temp |= (excepts & FE_ALL_EXCEPT) << FPC_FLAGS_SHIFT;
  _FPU_SETCW (temp);

  return 0;
}

static __always_inline int
fenv_setexceptflag (const fexcept_t *flagp, int excepts)
{
  fexcept_t fpc, fpc_new;

  /* Get the current environment.  We have to do this since we cannot
     separately set the status word.  */
  _FPU_GETCW (fpc);

  /* Clear the current exception bits.  */
  fpc_new = fpc & ~((excepts & FE_ALL_EXCEPT) << FPC_FLAGS_SHIFT);
  if ((fpc & FPC_NOT_FPU_EXCEPTION) == 0)
    /* Bits 6, 7 of dxc-byte are zero,
       thus bits 0-5 of dxc-byte correspond to the flag-bits.
       Clear given exceptions in dxc-field.  */
    fpc_new &= ~((excepts & FE_ALL_EXCEPT) << FPC_DXC_SHIFT);

  /* Set exceptions from flagp in flags-field.  */
  fpc_new |= (*flagp & excepts & FE_ALL_EXCEPT) << FPC_FLAGS_SHIFT;

  /* Store the new status word (along with the rest of the environment.
     Possibly new exceptions are set but they won't get executed.  */
  _FPU_SETCW (fpc_new);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_testexcept (int excepts)
{
  int res;
  fexcept_t fpc;

  _FPU_GETCW (fpc);

  /* Get current exceptions.  */
  res = (fpc >> FPC_FLAGS_SHIFT) & FE_ALL_EXCEPT;
  if ((fpc & FPC_NOT_FPU_EXCEPTION) == 0)
    /* Bits 6, 7 of dxc-byte are zero,
       thus bits 0-5 of dxc-byte correspond to the flag-bits.
       Evaluate flags and last dxc-exception-code.  */
    res |= (fpc >> FPC_DXC_SHIFT) & FE_ALL_EXCEPT;

  return res & excepts;
}

static __always_inline int
fenv_getround (void)
{
  return get_rounding_mode ();
}

static __always_inline int
fenv_setround (int round)
{
  if ((round | FPC_RM_MASK) != FPC_RM_MASK)
    {
      /* ROUND is not a valid rounding mode.  */
      return 1;
    }

  libc_fesetround_s390 (round);
  return 0;
}

static __always_inline int
fenv_getenv (fenv_t *envp)
{
  _FPU_GETCW (envp->__fpc);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_holdexcept (fenv_t *envp)
{
  fpu_control_t fpc, fpc_new;

  /* Store the environment.  */
  _FPU_GETCW (fpc);
  envp->__fpc = fpc;

  /* Clear the current exception flags and dxc field.
     Hold from generating fpu exceptions temporarily.  */
  fpc_new = fpc & ~(FPC_FLAGS_MASK | FPC_DXC_MASK | FPC_EXCEPTION_MASK);

  /* Only set new environment if it has changed.  */
  if (fpc_new != fpc)
    _FPU_SETCW (fpc_new);

  return 0;
}

static __always_inline int
fenv_setenv (const fenv_t *envp)
{
  fenv_t env = libc_handle_user_fenv_s390 (envp);
  fenv_set_env (&env);

  /* Success.  */
  return 0;
}

static __always_inline int
fenv_updateenv (const fenv_t *envp)
{
  fenv_t env = libc_handle_user_fenv_s390 (envp);
  s390_feupdateenv (&env);

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
  fpu_control_t fpc;

  _FPU_GETCW (fpc);
  fpc &= FPC_STATUS;
  if (modep == FE_DFL_MODE)
    fpc |= _FPU_DEFAULT;
  else
    fpc |= *modep & ~FPC_STATUS;
  _FPU_SETCW (fpc);

  return 0;
}

#define FENV_IMPL_HAVE_ISO_C 1

static __always_inline int
fenv_disableexcept (int excepts)
{
  fexcept_t temp, old_exc, new_flags;

  _FPU_GETCW (temp);
  old_exc = (temp & FPC_EXCEPTION_MASK) >> FPC_EXCEPTION_MASK_SHIFT;
  new_flags = (temp & ~(((unsigned int) excepts & FE_ALL_EXCEPT)
			<< FPC_EXCEPTION_MASK_SHIFT));
  _FPU_SETCW (new_flags);

  return old_exc;
}

static __always_inline int
fenv_enableexcept (int excepts)
{
  fexcept_t temp, old_exc, new_flags;

  _FPU_GETCW (temp);
  old_exc = (temp & FPC_EXCEPTION_MASK) >> FPC_EXCEPTION_MASK_SHIFT;
  new_flags = (temp | (((unsigned int) excepts & FE_ALL_EXCEPT)
		       << FPC_EXCEPTION_MASK_SHIFT));
  _FPU_SETCW (new_flags);

  return old_exc;
}

static __always_inline int
fenv_getexcept (void)
{
  fexcept_t exc;

  _FPU_GETCW (exc);
  return ((exc & FPC_EXCEPTION_MASK) >>  FPC_EXCEPTION_MASK_SHIFT);
}

#define FENV_IMPL_HAVE_TRAP_ENABLE 1

#include_next <fenv-impl.h>

#endif /* fenv-impl.h */
