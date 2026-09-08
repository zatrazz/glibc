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

/* The srnm instruction sets the rounding mode without rewriting the rest
   of the FPC, which fenv_set_control cannot express.  */
static __always_inline void
libc_feholdsetround_s390_ctx (struct rm_ctx *ctx, int r)
{
  fenv_get_control (&ctx->env);

  /* Set the rounding mode if changed.  */
  if (__glibc_unlikely (fenv_env_round (&ctx->env) != r))
    {
      ctx->updated_status = true;
      libc_fesetround_s390 (r);
    }
  else
    ctx->updated_status = false;
}

#define libc_feholdsetround_ctx		libc_feholdsetround_s390_ctx

/* The no-exception context also disables the exception traps while in
   the block, which the generic hook does not; the environment restore
   in libc_feresetround_noex_ctx reenables them.  */
static __always_inline void
libc_feholdsetround_noex_s390_ctx (struct rm_ctx *ctx, int r)
{
  fenv_t new;

  fenv_get_env (&ctx->env);
  new = ctx->env;
  fenv_env_clear_except (&new);
  fenv_env_clear_traps (&new);
  fenv_env_set_round (&new, r);
  fenv_update_env (&ctx->env, &new);
}

#define libc_feholdsetround_noex_ctx	libc_feholdsetround_noex_s390_ctx

#include_next <fenv_private.h>

#endif
