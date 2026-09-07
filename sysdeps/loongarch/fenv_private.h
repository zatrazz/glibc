/* Internal math stuff.
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

#ifndef LOONGARCH_FENV_PRIVATE_H
#define LOONGARCH_FENV_PRIVATE_H 1

/* Inline functions to speed up the math library implementation.  The
   default versions of these routines are in generic/fenv_private.h
   and call fesetround, feholdexcept, etc.  These routines use inlined
   code instead.  */

#ifdef __loongarch_hard_float

#include <fenv.h>
#include <fenv-impl.h>
#include <fenv_libc.h>
#include <fpu_control.h>

static __always_inline void
libc_fesetround_loongarch (int round)
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
#define libc_fesetround libc_fesetround_loongarch

static __always_inline void
libc_feholdexcept_setround_loongarch (fenv_t *envp, int round)
{
  fpu_control_t cw;

  /* Save the current state.  */
  _FPU_GETCW (cw);
  envp->__fp_control_register = cw;

  /* Clear all exception enable bits and flags.  */
  cw &= ~(_FPU_MASK_ALL);

  /* Set rounding bits.  */
  cw &= ~_FPU_RC_MASK;
  cw |= round;

  /* Set new state.  */
  _FPU_SETCW (cw);
}
#define libc_feholdexcept_setround libc_feholdexcept_setround_loongarch

#define libc_feholdsetround libc_feholdexcept_setround_loongarch

static __always_inline void
libc_fesetenv_loongarch (fenv_t *envp)
{
  fpu_control_t cw __attribute__ ((unused));

  /* Read current state to flush fpu pipeline.  */
  _FPU_GETCW (cw);

  _FPU_SETCW (envp->__fp_control_register);
}
#define libc_fesetenv libc_fesetenv_loongarch

static __always_inline int
libc_feupdateenv_test_loongarch (fenv_t *envp, int excepts)
{
  /* int ret = fetestexcept (excepts); feupdateenv (envp); return ret; */
  int held_ex, cw = envp->__fp_control_register;

  /* Get the current flag, i.e. all exceptions raised since we started
     to hold the exceptions.  We don't care CAUSE.  */
  _FPU_GET_FLAGS_CAUSE (held_ex);

  /* Set flag bits (which are accumulative).  */
  cw |= held_ex;
  _FPU_SETCW (cw);

  /* Raise SIGFPE for any new exceptions since the hold, in case any is
     enabled.  */
  if (__glibc_unlikely (((cw & ENABLE_MASK) << ENABLE_SHIFT) & held_ex))
    __feraiseexcept (held_ex);

  return cw & excepts & FE_ALL_EXCEPT;
}
#define libc_feupdateenv_test libc_feupdateenv_test_loongarch

static __always_inline void
libc_feupdateenv_loongarch (fenv_t *envp)
{
  libc_feupdateenv_test_loongarch (envp, 0);
}
#define libc_feupdateenv libc_feupdateenv_loongarch

#define libc_feresetround libc_feupdateenv_loongarch

/*  Enable support for rounding mode context.  */
#define HAVE_RM_CTX 1

static __always_inline void
libc_feholdexcept_setround_loongarch_ctx (struct rm_ctx *ctx, int round)
{
  fpu_control_t old, new;

  /* Save the current state.  */
  _FPU_GETCW (old);
  ctx->env.__fp_control_register = old;

  /* Clear all exception enable bits and flags.  */
  new = old & ~(_FPU_MASK_ALL);

  /* Set rounding bits.  */
  new = (new & ~_FPU_RC_MASK) | round;

  if (__glibc_unlikely (new != old))
    {
      _FPU_SETCW (new);
      ctx->updated_status = true;
    }
  else
    ctx->updated_status = false;
}
#define libc_feholdexcept_setround_ctx libc_feholdexcept_setround_loongarch_ctx

static __always_inline void
libc_fesetenv_loongarch_ctx (struct rm_ctx *ctx)
{
  libc_fesetenv_loongarch (&ctx->env);
}
#define libc_fesetenv_ctx libc_fesetenv_loongarch_ctx

static __always_inline void
libc_feupdateenv_loongarch_ctx (struct rm_ctx *ctx)
{
  if (__glibc_unlikely (ctx->updated_status))
    libc_feupdateenv_test_loongarch (&ctx->env, 0);
}
#define libc_feupdateenv_ctx libc_feupdateenv_loongarch_ctx
#define libc_feresetround_ctx libc_feupdateenv_loongarch_ctx

static __always_inline void
libc_feholdsetround_loongarch_ctx (struct rm_ctx *ctx, int round)
{
  fpu_control_t old, new;

  /* Save the current state.  */
  _FPU_GETCW (old);
  ctx->env.__fp_control_register = old;

  /* Set rounding bits.  */
  new = (old & ~_FPU_RC_MASK) | round;

  if (__glibc_unlikely (new != old))
    {
      _FPU_SETCW (new);
      ctx->updated_status = true;
    }
  else
    ctx->updated_status = false;
}
#define libc_feholdsetround_ctx libc_feholdsetround_loongarch_ctx

#endif

#include_next <fenv_private.h>

#endif
