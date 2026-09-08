/* Optimized inline fenv.h functions for libm.  Generic version.
   Copyright (C) 2011-2026 Free Software Foundation, Inc.
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

#ifndef _FENV_PRIVATE_H
#define _FENV_PRIVATE_H 1

#include <fenv.h>
#include <fenv-impl.h>
#include <get-rounding-mode.h>

/* The standards only specify one variant of the fenv.h interfaces.
   But at least for some architectures we can be more efficient if we
   know what operations are going to be performed.  Therefore we
   define additional interfaces.  By default they refer to the normal
   interfaces.  Each interface comes in a float (f) and a long double (l)
   variant as well, for the architectures where the floating-point state
   depends on the type; by default they refer to the unsuffixed one.

   The architecture <fenv-impl.h> provides the normal interfaces as inline
   functions, and the default versions below use them.  An architecture
   <fenv_private.h> may define FENV_PRIVATE_OUT_OF_LINE to use the
   out-of-line functions instead, for instance when the inline functions
   carry run time checks for the floating-point unit, or when the
   floating-point environment lives in memory the compiler does not know
   the arithmetic routines update.  */

#ifndef FENV_PRIVATE_OUT_OF_LINE
# define fenv_private_fegetenv(e) fenv_getenv (e)
# define fenv_private_feholdexcept(e) fenv_holdexcept (e)
# define fenv_private_fesetround(r) fenv_setround (r)
# define fenv_private_fetestexcept(x) fenv_testexcept (x)
# define fenv_private_fesetenv(e) fenv_setenv (e)
# define fenv_private_feupdateenv(e) fenv_updateenv (e)
#else
# define fenv_private_fegetenv(e) __fegetenv (e)
# define fenv_private_feholdexcept(e) __feholdexcept (e)
# define fenv_private_fesetround(r) __fesetround (r)
# define fenv_private_fetestexcept(x) __fetestexcept (x)
# define fenv_private_fesetenv(e) __fesetenv (e)
# define fenv_private_feupdateenv(e) __feupdateenv (e)
#endif

static __always_inline void
default_libc_feholdexcept (fenv_t *e)
{
  (void) fenv_private_feholdexcept (e);
}

#ifndef libc_feholdexcept
# define libc_feholdexcept  default_libc_feholdexcept
#endif
#ifndef libc_feholdexceptf
# define libc_feholdexceptf libc_feholdexcept
#endif
#ifndef libc_feholdexceptl
# define libc_feholdexceptl libc_feholdexcept
#endif

static __always_inline void
default_libc_feholdexcept_setround (fenv_t *e, int r)
{
  fenv_private_feholdexcept (e);
  fenv_private_fesetround (r);
}

#ifndef libc_feholdexcept_setround
# define libc_feholdexcept_setround  default_libc_feholdexcept_setround
#endif
#ifndef libc_feholdexcept_setroundf
# define libc_feholdexcept_setroundf libc_feholdexcept_setround
#endif
#ifndef libc_feholdexcept_setroundl
# define libc_feholdexcept_setroundl libc_feholdexcept_setround
#endif

static __always_inline void
default_libc_fesetenv (fenv_t *e)
{
  (void) fenv_private_fesetenv (e);
}

#ifndef libc_fesetenv
# define libc_fesetenv  default_libc_fesetenv
#endif
#ifndef libc_fesetenvf
# define libc_fesetenvf libc_fesetenv
#endif
#ifndef libc_fesetenvl
# define libc_fesetenvl libc_fesetenv
#endif

static __always_inline int
default_libc_feupdateenv_test (fenv_t *e, int ex)
{
  int ret = fenv_private_fetestexcept (ex);
  fenv_private_feupdateenv (e);
  return ret;
}

#ifndef libc_feupdateenv_test
# define libc_feupdateenv_test  default_libc_feupdateenv_test
#endif
#ifndef libc_feupdateenv_testf
# define libc_feupdateenv_testf libc_feupdateenv_test
#endif
#ifndef libc_feupdateenv_testl
# define libc_feupdateenv_testl libc_feupdateenv_test
#endif

#ifndef HAVE_RM_CTX
# define HAVE_RM_CTX 0
#endif

/* Default implementation using standard fenv functions.
   Avoid unnecessary rounding mode changes by first checking the
   current rounding mode.  Note the use of __glibc_unlikely is
   important for performance.  */

static __always_inline void
default_libc_feholdsetround_ctx (struct rm_ctx *ctx, int round)
{
  ctx->updated_status = false;

  /* Update rounding mode only if different.  */
  if (__glibc_unlikely (round != get_rounding_mode ()))
    {
      ctx->updated_status = true;
      fenv_private_fegetenv (&ctx->env);
      fenv_private_fesetround (round);
    }
}

static __always_inline void
default_libc_feresetround_ctx (struct rm_ctx *ctx)
{
  /* Restore the rounding mode if updated.  */
  if (__glibc_unlikely (ctx->updated_status))
    fenv_private_feupdateenv (&ctx->env);
}

static __always_inline void
default_libc_feholdsetround_noex_ctx (struct rm_ctx *ctx, int round)
{
  /* Save exception flags and rounding mode, and disable exception
     traps.  */
  fenv_private_feholdexcept (&ctx->env);

  /* Update rounding mode only if different.  */
  if (__glibc_unlikely (round != get_rounding_mode ()))
    fenv_private_fesetround (round);
}

static __always_inline void
default_libc_feresetround_noex_ctx (struct rm_ctx *ctx)
{
  /* Restore exception flags and rounding mode.  */
  fenv_private_fesetenv (&ctx->env);
}

#if HAVE_RM_CTX
/* Set/Restore Rounding Modes only when necessary.  If defined, these functions
   set/restore floating point state only if the state needed within the lexical
   block is different from the current state.  This saves a lot of time when
   the floating point unit is much slower than the fixed point units.  */

# ifndef libc_feholdsetroundf_ctx
#   define libc_feholdsetroundf_ctx libc_feholdsetround_ctx
# endif
# ifndef libc_feholdsetroundl_ctx
#   define libc_feholdsetroundl_ctx libc_feholdsetround_ctx
# endif
# ifndef libc_feresetroundf_ctx
#   define libc_feresetroundf_ctx libc_feresetround_ctx
# endif
# ifndef libc_feresetroundl_ctx
#   define libc_feresetroundl_ctx libc_feresetround_ctx
# endif

# ifndef libc_feholdsetround_noex_ctx
#   define libc_feholdsetround_noex_ctx  libc_feholdsetround_ctx
# endif
# ifndef libc_feholdsetround_noexf_ctx
#   define libc_feholdsetround_noexf_ctx libc_feholdsetround_noex_ctx
# endif
# ifndef libc_feholdsetround_noexl_ctx
#   define libc_feholdsetround_noexl_ctx libc_feholdsetround_noex_ctx
# endif

# ifndef libc_feresetround_noex_ctx
#   define libc_feresetround_noex_ctx  libc_fesetenv_ctx
# endif
# ifndef libc_feresetround_noexf_ctx
#   define libc_feresetround_noexf_ctx libc_feresetround_noex_ctx
# endif
# ifndef libc_feresetround_noexl_ctx
#   define libc_feresetround_noexl_ctx libc_feresetround_noex_ctx
# endif

#else

# define libc_feholdsetround_ctx      default_libc_feholdsetround_ctx
# define libc_feresetround_ctx        default_libc_feresetround_ctx
# define libc_feholdsetround_noex_ctx default_libc_feholdsetround_noex_ctx
# define libc_feresetround_noex_ctx   default_libc_feresetround_noex_ctx

# define libc_feholdsetroundf_ctx libc_feholdsetround_ctx
# define libc_feholdsetroundl_ctx libc_feholdsetround_ctx
# define libc_feresetroundf_ctx   libc_feresetround_ctx
# define libc_feresetroundl_ctx   libc_feresetround_ctx

# define libc_feholdsetround_noexf_ctx libc_feholdsetround_noex_ctx
# define libc_feholdsetround_noexl_ctx libc_feholdsetround_noex_ctx
# define libc_feresetround_noexf_ctx   libc_feresetround_noex_ctx
# define libc_feresetround_noexl_ctx   libc_feresetround_noex_ctx

#endif

#ifndef libc_feholdsetround_53bit_ctx
#  define libc_feholdsetround_53bit_ctx libc_feholdsetround_ctx
#endif
#ifndef libc_feresetround_53bit_ctx
#  define libc_feresetround_53bit_ctx libc_feresetround_ctx
#endif

#define SET_RESTORE_ROUND_GENERIC(RM,ROUNDFUNC,CLEANUPFUNC) \
  struct rm_ctx ctx __attribute__((cleanup (CLEANUPFUNC ## _ctx))); \
  ROUNDFUNC ## _ctx (&ctx, (RM))

/* Set the rounding mode within a lexical block.  Restore the rounding mode to
   the value at the start of the block.  The exception mode must be preserved.
   Exceptions raised within the block must be set in the exception flags.
   Non-stop mode may be enabled inside the block.  */

#define SET_RESTORE_ROUND(RM) \
  SET_RESTORE_ROUND_GENERIC (RM, libc_feholdsetround, libc_feresetround)
#define SET_RESTORE_ROUNDF(RM) \
  SET_RESTORE_ROUND_GENERIC (RM, libc_feholdsetroundf, libc_feresetroundf)
#define SET_RESTORE_ROUNDL(RM) \
  SET_RESTORE_ROUND_GENERIC (RM, libc_feholdsetroundl, libc_feresetroundl)

/* Set the rounding mode within a lexical block.  Restore the rounding mode to
   the value at the start of the block.  The exception mode must be preserved.
   Exceptions raised within the block must be discarded, and exception flags
   are restored to the value at the start of the block.
   Non-stop mode must be enabled inside the block.  */

#define SET_RESTORE_ROUND_NOEX(RM) \
  SET_RESTORE_ROUND_GENERIC (RM, libc_feholdsetround_noex, \
			     libc_feresetround_noex)
#define SET_RESTORE_ROUND_NOEXF(RM) \
  SET_RESTORE_ROUND_GENERIC (RM, libc_feholdsetround_noexf, \
			     libc_feresetround_noexf)
#define SET_RESTORE_ROUND_NOEXL(RM) \
  SET_RESTORE_ROUND_GENERIC (RM, libc_feholdsetround_noexl, \
			     libc_feresetround_noexl)

/* Like SET_RESTORE_ROUND, but also set rounding precision to 53 bits.  */
#define SET_RESTORE_ROUND_53BIT(RM) \
  SET_RESTORE_ROUND_GENERIC (RM, libc_feholdsetround_53bit,	      \
			     libc_feresetround_53bit)

#endif /* fenv_private.h.  */
