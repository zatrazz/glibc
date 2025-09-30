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

static __always_inline float
sqrtf_impl (float x)
{
  float z;
  int32_t sign = (int) 0x80000000;
  int32_t ix, s, q, m, t, i;
  uint32_t r;

  GET_FLOAT_WORD (ix, x);

  /* take care of Inf and NaN */
  if ((ix & 0x7f800000) == 0x7f800000)
    {
      if (ix == 0xff800000)
	return __math_invalidf (0.0f);
      return x * x + x; /* sqrt(NaN)=NaN, sqrt(+inf)=+inf
			   sqrt(-inf)=sNaN */
    }
  /* take care of zero */
  if (ix <= 0)
    {
      if ((ix & (~sign)) == 0)
	return x; /* sqrt(+-0) = +-0 */
      else if (ix < 0)
	return __math_invalidf (0.0f); /* sqrt(-ve) = sNaN */
    }
  /* normalize x */
  m = (ix >> 23);
  if (m == 0)
    { /* subnormal x */
      for (i = 0; (ix & 0x00800000) == 0; i++)
	ix <<= 1;
      m -= i - 1;
    }
  m -= 127; /* unbias exponent */
  ix = (ix & 0x007fffff) | 0x00800000;
  if (m & 1) /* odd m, double x to make it even */
    ix += ix;
  m >>= 1; /* m = [m/2] */

  /* generate sqrt(x) bit by bit */
  ix += ix;
  q = s = 0;	  /* q = sqrt(x) */
  r = 0x01000000; /* r = moving bit from right to left */

  while (r != 0)
    {
      t = s + r;
      if (t <= ix)
	{
	  s = t + r;
	  ix -= t;
	  q += r;
	}
      ix += ix;
      r >>= 1;
    }

  /* use floating add to find out rounding direction */
  if (ix != 0)
    {
      z = 0x1p0 - 0x1.4484cp-100; /* trigger inexact flag.  */
      if (z >= 0x1p0)
	{
	  z = 0x1p0 + 0x1.4484cp-100;
	  if (z > 0x1p0)
	    q += 2;
	  else
	    q += (q & 1);
	}
    }
  ix = (q >> 1) + 0x3f000000;
  ix += (m << 23);
  SET_FLOAT_WORD (z, ix);
  return z;
}
