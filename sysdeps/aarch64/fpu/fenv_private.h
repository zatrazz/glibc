/* Private floating point rounding and exceptions handling.  AArch64 version.
   Copyright (C) 2014-2026 Free Software Foundation, Inc.
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

#ifndef AARCH64_FENV_PRIVATE_H
#define AARCH64_FENV_PRIVATE_H 1

#include <fenv.h>
#include <fenv-impl.h>
#include <fpu_control.h>

#define libc_fesetround  libc_fesetround_aarch64

static __always_inline void
libc_feholdexcept_setround_aarch64 (fenv_t *envp, int round)
{
  fpu_control_t fpcr;
  fpu_control_t new_fpcr;
  fpu_fpsr_t fpsr;
  fpu_fpsr_t new_fpsr;

  _FPU_GETCW (fpcr);
  _FPU_GETFPSR (fpsr);
  envp->__fpcr = fpcr;
  envp->__fpsr = fpsr;

  /* Clear exception flags, set all exceptions to non-stop,
     and set new rounding mode.  */
  new_fpcr = fpcr & ~((FE_ALL_EXCEPT << FE_EXCEPT_SHIFT) | _FPU_FPCR_RM_MASK);
  new_fpcr |= round;
  new_fpsr = fpsr & ~FE_ALL_EXCEPT;

  if (__glibc_unlikely (new_fpcr != fpcr))
    _FPU_SETCW (new_fpcr);

  if (new_fpsr != fpsr)
    _FPU_SETFPSR (new_fpsr);
}

#define libc_feholdexcept_setround  libc_feholdexcept_setround_aarch64

#define libc_fesetenv  libc_fesetenv_aarch64
#define libc_feresetround_noex  libc_fesetenv_aarch64

#define libc_feupdateenv_test  libc_feupdateenv_test_aarch64

#define libc_feupdateenv  libc_feupdateenv_aarch64

static __always_inline void
libc_feholdsetround_aarch64 (fenv_t *envp, int round)
{
  fpu_control_t fpcr;
  fpu_fpsr_t fpsr;

  _FPU_GETCW (fpcr);
  _FPU_GETFPSR (fpsr);
  envp->__fpcr = fpcr;
  envp->__fpsr = fpsr;

  /* Check whether rounding modes are different.  */
  round = (fpcr ^ round) & _FPU_FPCR_RM_MASK;

  /* Set new rounding mode if different.  */
  if (__glibc_unlikely (round != 0))
    _FPU_SETCW (fpcr ^ round);
}

#define libc_feholdsetround  libc_feholdsetround_aarch64

static __always_inline void
libc_feresetround_aarch64 (fenv_t *envp)
{
  fpu_control_t fpcr;
  int round;

  _FPU_GETCW (fpcr);

  /* Check whether rounding modes are different.  */
  round = (envp->__fpcr ^ fpcr) & _FPU_FPCR_RM_MASK;

  /* Restore the rounding mode if it was changed.  */
  if (__glibc_unlikely (round != 0))
    _FPU_SETCW (fpcr ^ round);
}

#define libc_feresetround  libc_feresetround_aarch64

/* We have support for rounding mode context.  */
#define HAVE_RM_CTX 1

static __always_inline void
libc_feholdsetround_aarch64_ctx (struct rm_ctx *ctx, int r)
{
  fpu_control_t fpcr;
  int round;

  _FPU_GETCW (fpcr);
  ctx->env.__fpcr = fpcr;

  /* Check whether rounding modes are different.  */
  round = (fpcr ^ r) & _FPU_FPCR_RM_MASK;
  ctx->updated_status = round != 0;

  /* Set the rounding mode if changed.  */
  if (__glibc_unlikely (round != 0))
    _FPU_SETCW (fpcr ^ round);
}

#define libc_feholdsetround_ctx		libc_feholdsetround_aarch64_ctx

static __always_inline void
libc_feresetround_aarch64_ctx (struct rm_ctx *ctx)
{
  /* Restore the rounding mode if updated.  */
  if (__glibc_unlikely (ctx->updated_status))
    _FPU_SETCW (ctx->env.__fpcr);
}

#define libc_feresetround_ctx		libc_feresetround_aarch64_ctx

static __always_inline void
libc_feholdsetround_noex_aarch64_ctx (struct rm_ctx *ctx, int r)
{
  fpu_control_t fpcr;
  fpu_fpsr_t fpsr;
  int round;

  _FPU_GETCW (fpcr);
  _FPU_GETFPSR (fpsr);
  ctx->env.__fpcr = fpcr;
  ctx->env.__fpsr = fpsr;

  /* Check whether rounding modes are different.  */
  round = (fpcr ^ r) & _FPU_FPCR_RM_MASK;
  ctx->updated_status = round != 0;

  /* Set the rounding mode if changed.  */
  if (__glibc_unlikely (round != 0))
    _FPU_SETCW (fpcr ^ round);
}

#define libc_feholdsetround_noex_ctx	libc_feholdsetround_noex_aarch64_ctx

static __always_inline void
libc_feresetround_noex_aarch64_ctx (struct rm_ctx *ctx)
{
  /* Restore the rounding mode if updated.  */
  if (__glibc_unlikely (ctx->updated_status))
    _FPU_SETCW (ctx->env.__fpcr);

  /* Write new FPSR to restore exception flags.  */
  _FPU_SETFPSR (ctx->env.__fpsr);
}

#define libc_feresetround_noex_ctx	libc_feresetround_noex_aarch64_ctx

#include_next <fenv_private.h>

#endif
