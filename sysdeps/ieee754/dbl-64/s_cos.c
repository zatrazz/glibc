/*
 * IBM Accurate Mathematical Library
 * written by International Business Machines Corp.
 * Copyright (C) 2001-2025 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU  Lesser General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */
/****************************************************************************/
/*                                                                          */
/* MODULE_NAME:usncs.c                                                      */
/*                                                                          */
/* FUNCTIONS: uscos                                                         */
/* FILES NEEDED: dla.h endian.h mpa.h mydefs.h  usncs.h                     */
/*		 branred.c sincos.tbl					    */
/*                                                                          */
/* An ultimate sin and cos routine. Given an IEEE double machine number x   */
/* it computes sin(x) or cos(x) with ~0.55 ULP.				    */
/* Assumption: Machine arithmetic operations are performed in               */
/* round to nearest mode of IEEE 754 standard.                              */
/*                                                                          */
/****************************************************************************/

#include <errno.h>
#include <float.h>
#include "endian.h"
#include "mydefs.h"
#include "usncs.h"
#include <math.h>
#include <math_private.h>
#include <fenv_private.h>
#include <math-underflow.h>
#include <libm-alias-double.h>
#include <fenv.h>

#ifndef SECTION
# define SECTION
#endif

/*******************************************************************/
/* An ultimate cos routine. Given an IEEE double machine number x  */
/* it computes the rounded value of cos(x).			   */
/*******************************************************************/

double
SECTION
__cos (double x)
{
  double y, a, da;
  mynumber u;
  int4 k, m, n;

  double retval = 0;

  SET_RESTORE_ROUND_53BIT (FE_TONEAREST);

  u.x = x;
  m = u.i[HIGH_HALF];
  k = 0x7fffffff & m;

  /* |x|<2^-27 => cos(x)=1 */
  if (k < 0x3e400000)
    retval = 1.0;

  else if (k < 0x3feb6000)
    {				/* 2^-27 < |x| < 0.855469 */
      /* Max ULP is 0.51.  */
      retval = do_cos (x, 0);
    }				/*   else  if (k < 0x3feb6000)    */

  else if (k < 0x400368fd)
    { /* 0.855469  <|x|<2.426265  */ ;
      y = hp0 - fabs (x);
      a = y + hp1;
      da = (y - a) + hp1;
      /* Max ULP is 0.501 if xx < 0.01588 or 0.518 otherwise.
	 Range reduction uses 106 bits here which is sufficient.  */
      retval = do_sin (a, da);
    }				/*   else  if (k < 0x400368fd)    */

  else if (k < 0x419921FB)
    {				/* 2.426265<|x|< 105414350 */
      n = reduce_sincos (x, &a, &da);
      retval = do_sincos (a, da, n + 1);
    }				/*   else  if (k <  0x419921FB )    */

  /* 105414350 <|x| <2^1024 */
  else if (k < 0x7ff00000)
    {
      n = __branred (x, &a, &da);
      retval = do_sincos (a, da, n + 1);
    }

  else
    {
      if (k == 0x7ff00000 && u.i[LOW_HALF] == 0)
	__set_errno (EDOM);
      retval = x / x;		/* |x| > 2^1024 */
    }

  return retval;
}

#ifndef __cos
libm_alias_double (__cos, cos)
#endif
