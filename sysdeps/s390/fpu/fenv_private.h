/* Private floating point rounding and exceptions handling.  s390x version.
   Copyright (C) 2019-2026 Free Software Foundation, Inc.
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

#ifndef S390_FENV_PRIVATE_H
#define S390_FENV_PRIVATE_H 1

#include <fenv.h>
#include <fenv-impl.h>
#include <fenv_libc.h>
#include <fpu_control.h>

#define libc_fesetround  libc_fesetround_s390

static __always_inline void
libc_feholdexcept_setround_s390 (fenv_t *envp, int r)
{
  fpu_control_t fpc, fpc_new;

  _FPU_GETCW (fpc);
  envp->__fpc = fpc;

  /* Clear the current exception flags and dxc field.
     Hold from generating fpu exceptions temporarily.
     Reset rounding mode bits.  */
  fpc_new = fpc & ~(FPC_FLAGS_MASK | FPC_DXC_MASK | FPC_EXCEPTION_MASK
		    | FPC_RM_MASK);

  /* Set new rounding mode.  */
  fpc_new |= (r & FPC_RM_MASK);

  /* Only set new environment if it has changed.  */
  if (fpc_new != fpc)
    _FPU_SETCW (fpc_new);
}

#define libc_feholdexcept_setround  libc_feholdexcept_setround_s390

#define libc_fesetenv  libc_fesetenv_s390

#define libc_feupdateenv_test  libc_feupdateenv_test_s390

#define libc_feupdateenv  libc_feupdateenv_s390

/* We have support for rounding mode context.  */
#define HAVE_RM_CTX 1

static __always_inline void
libc_feholdsetround_s390_ctx (struct rm_ctx *ctx, int r)
{
  fpu_control_t fpc;
  int round;

  _FPU_GETCW (fpc);
  ctx->env.__fpc = fpc;

  /* Check whether rounding modes are different.  */
  round = fpc & FPC_RM_MASK;

  /* Set the rounding mode if changed.  */
  if (__glibc_unlikely (round != r))
    {
      ctx->updated_status = true;
      libc_fesetround_s390 (r);
    }
  else
    ctx->updated_status = false;
}

#define libc_feholdsetround_ctx		libc_feholdsetround_s390_ctx

static __always_inline void
libc_feresetround_s390_ctx (struct rm_ctx *ctx)
{
  /* Restore the rounding mode if updated.  */
  if (__glibc_unlikely (ctx->updated_status))
    {
      fpu_control_t fpc;
      _FPU_GETCW (fpc);
      fpc = ctx->env.__fpc | (fpc & FPC_FLAGS_MASK);
      _FPU_SETCW (fpc);
    }
}

#define libc_feresetround_ctx		libc_feresetround_s390_ctx

static __always_inline void
libc_feholdsetround_noex_s390_ctx (struct rm_ctx *ctx, int r)
{
  libc_feholdexcept_setround_s390 (&ctx->env, r);
}

#define libc_feholdsetround_noex_ctx	libc_feholdsetround_noex_s390_ctx

static __always_inline void
libc_feresetround_noex_s390_ctx (struct rm_ctx *ctx)
{
  /* Restore exception flags and rounding mode.  */
  libc_fesetenv_s390 (&ctx->env);
}

#define libc_feresetround_noex_ctx	libc_feresetround_noex_s390_ctx

#include_next <fenv_private.h>

#endif
