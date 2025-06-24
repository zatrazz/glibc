/*
 * IBM Accurate Mathematical Library
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

/************************************************************************/
/*  MODULE_NAME: dosincos.h                                             */
/*                                                                      */
/*                                                                      */
/* 	common data and variables definition for BIG or LITTLE ENDIAN   */
/************************************************************************/

#ifndef USNCS_H
#define USNCS_H

#include <math.h>

static const double s1 = -0x1.5555555555555p-3;   /* -0.16666666666666666     */
static const double s2 = 0x1.1111111110ECEp-7;    /*  0.0083333333333323288   */
static const double s3 = -0x1.A01A019DB08B8p-13;  /* -0.00019841269834414642  */
static const double s4 = 0x1.71DE27B9A7ED9p-19;   /*  2.755729806860771e-06   */
static const double s5 = -0x1.ADDFFC2FCDF59p-26;  /* -2.5022014848318398e-08  */
static const double aa = -0x1.5558000000000p-3;   /* -0.1666717529296875      */
static const double bb = 0x1.5555555556E24p-18;   /*  5.0862630208387126e-06  */
static const double big = 0x1.8000000000000p45;   /*  52776558133248          */
static const double hp0 = 0x1.921FB54442D18p0;    /*  1.5707963267948966      */
static const double hp1 = 0x1.1A62633145C07p-54;  /*  6.123233995736766e-17   */
static const double mp1 = 0x1.921FB58000000p0;    /*  1.5707963407039642      */
static const double mp2 = -0x1.DDE973C000000p-27; /* -1.3909067564377153e-08  */
static const double mp3 = -0x1.CB3B399D747F2p-55; /* -4.9789962505147994e-17  */
static const double pp3 = -0x1.CB3B398000000p-55; /* -4.9789962314799099e-17  */
static const double pp4 = -0x1.d747f23e32ed7p-83; /* -1.9034889620193266e-25  */
static const double hpinv = 0x1.45F306DC9C883p-1; /*  0.63661977236758138     */
static const double toint = 0x1.8000000000000p52; /*  6755399441055744        */

static const double
  sn3 = -1.66666666666664880952546298448555E-01,
  sn5 = 8.33333214285722277379541354343671E-03,
  cs2 = 4.99999999999999999999950396842453E-01,
  cs4 = -4.16666666666664434524222570944589E-02,
  cs6 = 1.38888874007937613028114285595617E-03;


extern const union
{
  int4 i[880];
  double x[440];
} __sincostab attribute_hidden;

#define SINCOS_TABLE_LOOKUP(u, sn, ssn, cs, ccs) \
({									      \
  int4 k = u.i[LOW_HALF] << 2;						      \
  sn = __sincostab.x[k];						      \
  ssn = __sincostab.x[k + 1];						      \
  cs = __sincostab.x[k + 2];						      \
  ccs = __sincostab.x[k + 3];						      \
})


/* Helper macros to compute sin of the input values.  */
#define POLYNOMIAL2(xx) ((((s5 * (xx) + s4) * (xx) + s3) * (xx) + s2) * (xx))

#define POLYNOMIAL(xx) (POLYNOMIAL2 (xx) + s1)

/* The computed polynomial is a variation of the Taylor series expansion for
   sin(x):

   x - x^3/3! + x^5/5! - x^7/7! + x^9/9! - dx*x^2/2 + dx

   The constants s1, s2, s3, etc. are pre-computed values of 1/3!, 1/5! and so
   on.  The result is returned to LHS.  */
#define TAYLOR_SIN(xx, x, dx) \
({									      \
  double t = ((POLYNOMIAL (xx)  * (x) - 0.5 * (dx))  * (xx) + (dx));	      \
  double res = (x) + t;							      \
  res;									      \
})

extern int __branred (double x, double *a, double *aa) attribute_hidden;

/* Given a number partitioned into X and DX, this function computes the cosine
   of the number by combining the sin and cos of X (as computed by a variation
   of the Taylor series) with the values looked up from the sin/cos table to
   get the result.  */
static __always_inline double
do_cos (double x, double dx)
{
  mynumber u;

  if (x < 0)
    dx = -dx;

  u.x = big + fabs (x);
  x = fabs (x) - (u.x - big) + dx;

  double xx, s, sn, ssn, c, cs, ccs, cor;
  xx = x * x;
  s = x + x * xx * (sn3 + xx * sn5);
  c = xx * (cs2 + xx * (cs4 + xx * cs6));
  SINCOS_TABLE_LOOKUP (u, sn, ssn, cs, ccs);
  cor = (ccs - s * ssn - cs * c) - sn * s;
  return cs + cor;
}

/* Given a number partitioned into X and DX, this function computes the sine of
   the number by combining the sin and cos of X (as computed by a variation of
   the Taylor series) with the values looked up from the sin/cos table to get
   the result.  */
static __always_inline double
do_sin (double x, double dx)
{
  double xold = x;
  /* Max ULP is 0.501 if |x| < 0.126, otherwise ULP is 0.518.  */
  if (fabs (x) < 0.126)
    return TAYLOR_SIN (x * x, x, dx);

  mynumber u;

  if (x <= 0)
    dx = -dx;
  u.x = big + fabs (x);
  x = fabs (x) - (u.x - big);

  double xx, s, sn, ssn, c, cs, ccs, cor;
  xx = x * x;
  s = x + (dx + x * xx * (sn3 + xx * sn5));
  c = x * dx + xx * (cs2 + xx * (cs4 + xx * cs6));
  SINCOS_TABLE_LOOKUP (u, sn, ssn, cs, ccs);
  cor = (ssn + s * ccs - sn * c) + cs * s;
  return copysign (sn + cor, xold);
}

/* Reduce range of x to within PI/2 with abs (x) < 105414350.  The high part
   is written to *a, the low part to *da.  Range reduction is accurate to 136
   bits so that when x is large and *a very close to zero, all 53 bits of *a
   are correct.  */
static __always_inline int4
reduce_sincos (double x, double *a, double *da)
{
  mynumber v;

  double t = (x * hpinv + toint);
  double xn = t - toint;
  v.x = t;
  double y = (x - xn * mp1) - xn * mp2;
  int4 n = v.i[LOW_HALF] & 3;

  double b, db, t1, t2;
  t1 = xn * pp3;
  t2 = y - t1;
  db = (y - t2) - t1;

  t1 = xn * pp4;
  b = t2 - t1;
  db += (t2 - b) - t1;

  *a = b;
  *da = db;
  return n;
}

/* Compute sin or cos (A + DA) for the given quadrant N.  */
static __always_inline double
do_sincos (double a, double da, int4 n)
{
  double retval;

  if (n & 1)
    /* Max ULP is 0.513.  */
    retval = do_cos (a, da);
  else
    /* Max ULP is 0.501 if xx < 0.01588, otherwise ULP is 0.518.  */
    retval = do_sin (a, da);

  return (n & 2) ? -retval : retval;
}

#endif
