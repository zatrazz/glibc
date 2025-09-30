/* e_sqrtf.c -- float version of e_sqrt.c.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include <math.h>
#include <math_private.h>
#include <libm-alias-finite.h>
#include <libm-alias-float.h>
#include <math-svid-compat.h>
#include <math-use-builtins.h>
#include "math_config.h"
#include <e_sqrtf_impl.h>

float
__sqrtf (float x)
{
#if USE_SQRTF_BUILTIN
  if (__glibc_unlikely (isless (x, 0.0f)))
    return __math_invalidf (x);
  return __builtin_sqrtf (x);
#else
  /* Use generic implementation.  */
  return sqrtf_impl (x);
#endif /* ! USE_SQRTF_BUILTIN  */
}
#ifndef __ieee754_sqrtf
libm_alias_finite (__sqrtf, __sqrtf)
#endif
#if LIBM_SVID_COMPAT
versioned_symbol (libm, __sqrtf, sqrtf, GLIBC_2_43);
libm_alias_float_other (__sqrt, sqrt)
#else
libm_alias_float (__sqrt, sqrt)
#endif
