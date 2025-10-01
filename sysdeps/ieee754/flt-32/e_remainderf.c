/* e_remainderf.c -- float version of e_remainder.c.
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
#include "math_config.h"

static const float zero = 0.0;


float
__ieee754_remainderf(float x, float p)
{
	int32_t hx,hp;
	uint32_t sx;
	float p_half;

	GET_FLOAT_WORD(hx,x);
	GET_FLOAT_WORD(hp,p);
	sx = hx&0x80000000;
	hp &= 0x7fffffff;
	hx &= 0x7fffffff;

    /* purge off exception values */
	if((hp==0)||
	   ((hx==0x7f800000) && hp<=0x7f800000))
	  return __math_invalidf (x);
	if((hx>=0x7f800000)||			/* x not finite */
	  ((hp>0x7f800000)))			/* p is NaN */
	    return (x*p)/(x*p);


	if (hp<=0x7effffff) x = __ieee754_fmodf(x,p+p);	/* now x < 2p */
	if ((hx-hp)==0) return zero*x;
	x  = fabsf(x);
	p  = fabsf(p);
	if (hp<0x01000000) {
	    if(x+x>p) {
		x-=p;
		if(x+x>=p) x -= p;
	    }
	} else {
	    p_half = (float)0.5*p;
	    if(x>p_half) {
		x-=p;
		if(x>=p_half) x -= p;
	    }
	}
	GET_FLOAT_WORD(hx,x);
	/* Make sure x is not -0. This can occur only when x = p
	   and rounding direction is towards negative infinity. */
	if (hx==0x80000000) hx = 0;
	SET_FLOAT_WORD(x,hx^sx);
	return x;
}
libm_alias_finite (__ieee754_remainderf, __remainderf)
#if LIBM_SVID_COMPAT
versioned_symbol (libm, __ieee754_remainderf, remainderf, GLIBC_2_43);
libm_alias_float_other (__ieee754_remainder, remainder)
#else
libm_alias_float (__ieee754_remainder, remainder)
weak_alias (__ieee754_remainderf, dremf)
#endif
