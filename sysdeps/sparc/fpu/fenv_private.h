#ifndef SPARC_FENV_PRIVATE_H
#define SPARC_FENV_PRIVATE_H 1

#include <fenv.h>
#include <fenv-impl.h>
#include <fpu_control.h>

static __always_inline void
libc_feholdexcept_setround (fenv_t *e, int r)
{
  fenv_t etmp;
  __fenv_stfsr(etmp);
  *(e) = etmp;
  etmp = etmp & ~((0x1f << 23) | FE_ALL_EXCEPT);
  etmp = (etmp & ~__FE_ROUND_MASK) | (r);
  __fenv_ldfsr(etmp);
}

static __always_inline void
libc_fesetenv (fenv_t *e)
{
  __fenv_ldfsr(*e);
}

static __always_inline int
libc_feupdateenv_test (fenv_t *e, int ex)
{
  fenv_t etmp;

  __fenv_stfsr(etmp);
  etmp &= FE_ALL_EXCEPT;

  __fenv_ldfsr(*e);

  __feraiseexcept (etmp);

  return etmp & ex;
}

static __always_inline void
libc_feupdateenv (fenv_t *e)
{
  libc_feupdateenv_test (e, 0);
}

static __always_inline void
libc_feholdsetround (fenv_t *e, int r)
{
  fenv_t etmp;
  __fenv_stfsr(etmp);
  *(e) = etmp;
  etmp = (etmp & ~__FE_ROUND_MASK) | (r);
  __fenv_ldfsr(etmp);
}

static __always_inline void
libc_feresetround (fenv_t *e)
{
  fenv_t etmp;
  __fenv_stfsr(etmp);
  etmp = (etmp & ~__FE_ROUND_MASK) | (*e & __FE_ROUND_MASK);
  __fenv_ldfsr(etmp);
}

#define libc_fesetround			libc_fesetround
#define libc_feholdexcept_setround	libc_feholdexcept_setround
#define libc_fesetenv			libc_fesetenv
#define libc_feupdateenv_test		libc_feupdateenv_test
#define libc_feupdateenv		libc_feupdateenv
#define libc_feholdsetround		libc_feholdsetround
#define libc_feresetround		libc_feresetround

/* We have support for rounding mode context.  */
#define HAVE_RM_CTX 1

static __always_inline void
libc_feholdexcept_setround_sparc_ctx (struct rm_ctx *ctx, int round)
{
  fenv_t new;

  __fenv_stfsr(ctx->env);
  new = ctx->env & ~((0x1f << 23) | FE_ALL_EXCEPT);
  new = (new & ~__FE_ROUND_MASK) | round;
  if (__glibc_unlikely (new != ctx->env))
    {
      __fenv_ldfsr(new);
      ctx->updated_status = true;
    }
  else
    ctx->updated_status = false;
}

static __always_inline void
libc_fesetenv_sparc_ctx (struct rm_ctx *ctx)
{
  libc_fesetenv(&ctx->env);
}

static __always_inline void
libc_feupdateenv_sparc_ctx (struct rm_ctx *ctx)
{
  if (__glibc_unlikely (ctx->updated_status))
    libc_feupdateenv_test (&ctx->env, 0);
}

static __always_inline void
libc_feholdsetround_sparc_ctx (struct rm_ctx *ctx, int round)
{
  fenv_t new;

  __fenv_stfsr(ctx->env);
  new = (ctx->env & ~__FE_ROUND_MASK) | round;
  if (__glibc_unlikely (new != ctx->env))
    {
      __fenv_ldfsr(new);
      ctx->updated_status = true;
    }
  else
    ctx->updated_status = false;
}
#define libc_feholdexcept_setround_ctx   libc_feholdexcept_setround_sparc_ctx
#define libc_fesetenv_ctx                libc_fesetenv_sparc_ctx
#define libc_feupdateenv_ctx             libc_feupdateenv_sparc_ctx
#define libc_feresetround_ctx            libc_feupdateenv_sparc_ctx
#define libc_feholdsetround_ctx          libc_feholdsetround_sparc_ctx

#include_next <fenv_private.h>

#endif /* SPARC_FENV_PRIVATE_H */
