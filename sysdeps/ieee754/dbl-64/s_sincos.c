/* Correctly-rounded sincos for binary64 value.

Copyright (c) 2022-2024 Paul Zimmermann and Tom Hubrecht

This file is part of the CORE-MATH project
(https://core-math.gitlabpages.inria.fr/).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <fenv.h>
#include <errno.h>
#include <libm-alias-double.h>
#include "s_sincos.h"
#define CORE_MATH_SUPPORT_ERRNO

#ifndef SECTION
# define SECTION
#endif

/* Put in h+l an approximation of sin(x), and in hc+lc an approximation of cos(x),
   and return the maximal absolute error (for both).
   Adapted from sin_fast in sin.c */
static double
sincos_fast (double *h, double *l, double *hc, double *lc, double x)
{
  int neg = x < 0; // should we negate the sin value
  int negc;        // should we negate the cos value
  int is_sin = 1;  // does the reduce argument approximate sin or cos
  double absx = neg ? -x : x;

  /* now x > 0x1.7137449123ef6p-26 */
  double err1;
  int i = reduce_fast (h, l, absx, &err1);
  /* err1 is an absolute bound for | i/2^11 + h + l - frac(x/(2pi)) |:
     | i/2^11 + h + l - frac(x/(2pi)) | < err1 */

  // if i >= 2^10: 1/2 <= frac(x/(2pi)) < 1 thus pi <= x <= 2pi
  // we use sin(pi+x) = -sin(x), cos(pi+x) = -cos(x)
  neg = neg ^ (i >> 10);
  negc = i >> 10;
  i = i & 0x3ff;
  // | i/2^11 + h + l - frac(x/(2pi)) | mod 1/2 < err1

  // now i < 2^10
  // if i >= 2^9: 1/4 <= frac(x/(2pi)) < 1/2 thus pi/2 <= x <= pi
  // we use sin(pi/2+x) = cos(x), cos(pi/2+x) = -sin(x)
  is_sin = is_sin ^ (i >> 9);
  negc = negc ^ (i >> 9);
  i = i & 0x1ff;
  // | i/2^11 + h + l - frac(x/(2pi)) | mod 1/4 < err1

  // now 0 <= i < 2^9
  // if i >= 2^8: 1/8 <= frac(x/(2pi)) < 1/4
  // we use sin(pi/2-x) = cos(x), cos(pi/2-x) = sin(x)
  if (i & 0x100) // case pi/4 <= x_red <= pi/2
    {
      is_sin = !is_sin;
      i = 0x1ff - i;
      /* 0x1p-11 - h is exact below: indeed, reduce_fast first computes
         a first value of h (say h0, with 0 <= h0 < 1), then i = floor(h0*2^11)
         and h1 = h0 - 2^11*i with 0 <= h1 < 2^-11.
         If i >= 2^8 here, this implies h0 >= 1/2^3, thus ulp(h0) >= 2^-55:
         h0 and h1 are integer multiples of 2^-55.
         Thus h1 = k*2^-55 with 0 <= k < 2^44 (since 0 <= h1 < 2^-11).
         Then 0x1p-11 - h = (2^44-k)*2^-55 is exactly representable.
         We can have a huge cancellation in 0x1p-11 - h, for example for
         x = 0x1.61a3db8c8d129p+1023 where we have before this operation
         h = 0x1.ffffffffff8p-12, and h = 0x1p-53 afterwards. But this
         does not hurt since we bound the absolute error and not the
         relative error at the end. */
      *h = 0x1p-11 - *h;
      *l = -*l;
    }

  /* Now 0 <= i < 256 and 0 <= h+l < 2^-11
     with | i/2^11 + h + l - frac(x/(2pi)) | cmod 1/4 < err1
     If is_sin=1, sin |x| = sin2pi (R + err1);
     if is_sin=0, sin |x| = cos2pi (R + err1).
     In both cases R = i/2^11 + h + l, 0 <= R < 1/4.
  */
  double sh, sl, ch, cl;
  /* since the SC[] table evaluates at i/2^11 + SC[i][0] and not at i/2^11,
     we must subtract SC[i][0] from h+l */
  /* Here h = k*2^-55 with 0 <= k < 2^44, and SC[i][0] is an integer
     multiple of 2^-62, with |SC[i][0]| < 2^-24, thus SC[i][0] = m*2^-62
     with |m| < 2^38. It follows h-SC[i][0] = (k*2^7 + m)*2^-62 with
     2^51 - 2^38 < k*2^7 + m < 2^51 + 2^38, thus h-SC[i][0] is exact.
     Now |h| < 2^-11 + 2^-24. */
  *h -= SC[i][0];
  // now -2^-24 < h < 2^-11+2^-24
  // from reduce_fast() we have |l| < 2^-52.36
  double uh, ul;
  a_mul (&uh, &ul, *h, *h);
  ul = __builtin_fma (*h + *h, *l, ul);
  // uh+ul approximates (h+l)^2
  evalPSfast (&sh, &sl, *h, *l, uh, ul);
  /* the absolute error of evalPSfast() is less than 2^-77.09 from
     routine evalPSfast() in sin.sage:
     | sh + sh - sin2pi(h+l) | < 2^-77.09 */
  evalPCfast (&ch, &cl, uh, ul);
  /* the relative error of evalPCfast() is less than 2^-69.96 from
     routine evalPCfast(rel=true) in sin.sage:
     | ch + cl - cos2pi(h+l) | < 2^-69.96 * |ch + cl| */
  double Sh, Sl, Ch, Cl;
  static const double sgn[2] = {1.0, -1.0};
  if (is_sin)
    {
      /* sin(x) ~ SC[i][2] * (sh + sl) + SC[i][1] * (ch + cl) */
      /* cos(x) ~ SC[i][2] * (ch + cl) - SC[i][1] * (sh + sl) */
      s_mul (&Sh, &Sl, sgn[neg] * SC[i][2], sh, sl);
      s_mul (&Ch, &Cl, sgn[neg] * SC[i][1], ch, cl);
      fast_two_sum (h, l, Ch, Sh);
      *l += Sl + Cl;
      /* absolute error bounded by 2^-68.588
         from global_error(is_sin=true,rel=false) in sin.sage:
         | h + l - sin2pi (R) | < 2^-68.588
         thus:
         | h + l - sin |x| | < 2^-68.588 + | sin2pi (R) - sin |x| |
                             < 2^-68.588 + err1 */
      s_mul (&Ch, &Cl, sgn[negc] * SC[i][2], ch, cl);
      s_mul (&Sh, &Sl, sgn[negc] * SC[i][1], sh, sl);
      fast_two_sum (hc, lc, Ch, -Sh);
      *lc += Cl - Sl;
    }
  else
    {
      /* sin(x) ~ SC[i][2] * (ch + cl) - SC[i][1] * (sh + sl) */
      /* cos(x) ~ -SC[i][2] * (sh + sl) - SC[i][1] * (ch + cl) */
      s_mul (&Ch, &Cl, sgn[neg] * SC[i][2], ch, cl);
      s_mul (&Sh, &Sl, sgn[neg] * SC[i][1], sh, sl);
      fast_two_sum (h, l, Ch, -Sh);
      *l += Cl - Sl;
      /* absolute error bounded by 2^-68.414
         from global_error(is_sin=false,rel=false) in sin.sage:
         | h + l - cos2pi (R) | < 2^-68.414
         thus:
         | h + l - sin |x| | < 2^-68.414 + | cos2pi (R) - sin |x| |
                             < 2^-68.414 + err1 */
      s_mul (&Sh, &Sl, sgn[negc] * SC[i][2], sh, sl);
      s_mul (&Ch, &Cl, sgn[negc] * SC[i][1], ch, cl);
      fast_two_sum (hc, lc, Ch, Sh);
      *lc += Sl + Cl;
    }
  static const double err = 0x1.81p-69; // maximal error of both cases (see sin.c)
  return err + err1;
}

/* Assume x is a regular number, and |x| > 0x1.7137449123ef6p-26. */
static double
sin_accurate (double x)
{
  double absx = (x > 0) ? x : -x;

  dint64_t X[1];
  dint_fromd (X, absx, 0x3fe);

  /* reduce argument */
  reduce (X);
  
  // now |X - x/(2pi) mod 1| < 2^-126.67*X, with 0 <= X < 1.

  int neg = x < 0, is_sin = 1;

  // Write X = i/2^11 + r with 0 <= r < 2^11.
  int i = reduce2 (X); // exact

  if (i & 0x400) // pi <= x < 2*pi: sin(x) = -sin(x-pi)
  {
    neg = !neg;
    i = i & 0x3ff;
  }

  // now i < 2^10

  if (i & 0x200) // pi/2 <= x < pi: sin(x) = cos(x-pi/2)
  {
    is_sin = 0;
    i = i & 0x1ff;
  }

  // now 0 <= i < 2^9

  if (i & 0x100)
    // pi/4 <= x < pi/2: sin(x) = cos(pi/2-x), cos(x) = sin(pi/2-x)
  {
    is_sin = !is_sin;
    X->sgn = 1; // negate X
    add_dint (X, &MAGIC, X); // X -> 2^-11 - X
    // here: 256 <= i <= 511
    i = 0x1ff - i;
    // now 0 <= i < 256
  }

  // now 0 <= i < 256 and 0 <= X < 2^-11

  /* If is_sin=1, sin |x| = sin2pi (R * (1 + eps))
        (cases 0 <= x < pi/4 and 3pi/4 <= x < pi)
     if is_sin=0, sin |x| = cos2pi (R * (1 + eps))
        (case pi/4 <= x < 3pi/4)
     In both cases R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
  */

  dint64_t U[1], V[1], X2[1];
  mul_dint (X2, X, X);       // X2 approximates X^2
  evalPC (U, X2);    // cos2pi(X)
  /* since 0 <= X < 2^-11, we have 0.999 < U <= 1 */
  evalPS (V, X, X2); // sin2pi(X)
  /* since 0 <= X < 2^-11, we have 0 <= V < 0.0005 */
  if (is_sin)
  {
    // sin2pi(R) ~ sin2pi(i/2^11)*cos2pi(X)+cos2pi(i/2^11)*sin2pi(X)
    mul_dint (U, S+i, U);
    /* since 0 <= S[i] < 0.705 and 0.999 < Uin <= 1, we have
       0 <= U < 0.705 */
    mul_dint (V, C+i, V);
    /* For the error analysis, we distinguish the case i=0.
       For i=0, we have S[i]=0 and C[1]=1, thus V is the value computed
       by evalPS() above, with relative error < 2^-124.648.

       For 1 <= i < 256, analyze_sin_case1(rel=true) from sin.sage gives a
       relative error bound of -122.797 (obtained for i=1).
       In all cases, the relative error for the computation of
       sin2pi(i/2^11)*cos2pi(X)+cos2pi(i/2^11)*sin2pi(X) is bounded by -122.797
       not taking into account the approximation error in R:
       |U - sin2pi(R)| < |U| * 2^-122.797, with U the value computed
       after add_dint (U, U, V) below.

       For the approximation error in R, we have:
       sin |x| = sin2pi (R * (1 + eps))
       R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
       Thus sin|x| = sin2pi(R+R*eps)
                   = sin2pi(R)+R*eps*2*pi*cos2pi(theta), theta in [R,R+R*eps]
       Since 2*pi*R/sin(2*pi*R) < pi/2 for R < 1/4, it follows:
       | sin|x| - sin2pi(R) | < pi/2*R*|sin(2*pi*R)|
       | sin|x| - sin2pi(R) | < 2^-126.018 * |sin2pi(R)|.

       Adding both errors we get:
       | sin|x| - U | < |U| * 2^-122.797 + 2^-126.018 * |sin2pi(R)|
                      < |U| * 2^-122.797 + 2^-126.018 * |U| * (1 + 2^-122.797)
                      < |U| * 2^-122.650.
    */
  }
  else
  {
    // cos2pi(R) ~ cos2pi(i/2^11)*cos2pi(X)-sin2pi(i/2^11)*sin2pi(X)
    mul_dint (U, C+i, U);
    mul_dint (V, S+i, V);
    V->sgn = 1 - V->sgn; // negate V
    /* For 0 <= i < 256, analyze_sin_case2(rel=true) from sin.sage gives a
       relative error bound of -123.540 (obtained for i=0):
       |U - cos2pi(R)| < |U| * 2^-123.540, with U the value computed
       after add_dint (U, U, V) below.

       For the approximation error in R, we have:
       sin |x| = cos2pi (R * (1 + eps))
       R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
       Thus sin|x| = cos2pi(R+R*eps)
                   = cos2pi(R)-R*eps*2*pi*sin2pi(theta), theta in [R,R+R*eps]
       Since we have R < 1/4, we have cos2pi(R) >= sqrt(2)/2,
       and it follows:
       | sin|x|/cos2pi(R) - 1 | < 2*pi*R*eps/(sqrt(2)/2)
                                < pi/2*eps/sqrt(2)          [since R < 1/4]
                                < 2^-126.518.
       Adding both errors we get:
       | sin|x| - U | < |U| * 2^-123.540 + 2^-126.518 * |cos2pi(R)|
                      < |U| * 2^-123.540 + 2^-126.518 * |U| * (1 + 2^-123.540)
                      < |U| * 2^-123.367.
    */
  }
  add_dint (U, U, V);
  /* If is_sin=1:
     | sin|x| - U | < |U| * 2^-122.650
     If is_sin=0:
     | cos|x| - U | < |U| * 2^-123.367.
     In all cases the total error is bounded by |U| * 2^-122.650.
     The term |U| * 2^-122.650 contributes to at most 2^(128-122.650) < 41 ulps
     relatively to U->lo.
  */
  uint64_t err = 41;
  uint64_t hi0, hi1, lo0, lo1;
  lo0 = U->lo - err;
  hi0 = U->hi - (lo0 > U->lo);
  lo1 = U->lo + err;
  hi1 = U->hi + (lo1 < U->lo);
  /* check the upper 54 bits are equal */
  if ((hi0 >> 10) != (hi1 >> 10))
    {
      static const double exceptions[][3] = {
        {0x1.e0000000001c2p-20, 0x1.dfffffffff02ep-20, 0x1.dcba692492527p-146},
        /* the following worst case was reported by Erik E., it has 68
           identical bits after the round bit */
        {0x1.6ac5b262ca1ffp+849, 0x1p+0, -0x1.2b089ea1e692bp-123},
      };
      for (int j = 0; j < 2; j++)
        {
          if (__builtin_fabs (x) == exceptions[j][0])
            return (x > 0) ? exceptions[j][1] + exceptions[j][2]
              : -exceptions[j][1] - exceptions[j][2];
        }
      __builtin_unreachable ();
    }

  if (neg)
    U->sgn = 1 - U->sgn;

  double y = dint_tod (U);

  return y;
}

/* Assume x is a regular number and x > 0x1.6a09e667f3bccp-27. */
static double
cos_accurate (double x)
{
  dint64_t X[1];
  dint_fromd (X, x, 0x3fe);

  /* reduce argument */
  reduce (X);
  
  // now |X - x/(2pi) mod 1| < 2^-126.67*X, with 0 <= X < 1.

  int neg = 0, is_cos = 1;

  // Write X = i/2^11 + r with 0 <= r < 2^11.
  int i = reduce2 (X); // exact

  if (i & 0x400) // pi <= x < 2*pi: cos(x) = -cos(x-pi)
  {
    neg = 1;
    i = i & 0x3ff;
  }

  // now i < 2^10

  if (i & 0x200) // pi/2 <= x < pi: cos(x) = -sin(x-pi/2)
  {
    neg = !neg;
    is_cos = 0;
    i = i & 0x1ff;
  }

  // now 0 <= i < 2^9

  if (i & 0x100)
    // pi/4 <= x < pi/2: cos(x) = sin(pi/2-x), sin(x) = cos(pi/2-x)
  {
    is_cos = !is_cos;
    X->sgn = 1; // negate X
    add_dint (X, &MAGIC, X); // X -> 2^-11 - X
    // here: 256 <= i <= 511
    i = 0x1ff - i;
    // now 0 <= i < 256
  }

  // now 0 <= i < 256 and 0 <= X < 2^-11

  /* If is_cos=1, cos |x| = cos2pi (R * (1 + eps))
        (cases 0 <= x < pi/4 and 3pi/4 <= x < pi)
     if is_cos=0, cos |x| = sin2pi (R * (1 + eps))
        (case pi/4 <= x < 3pi/4)
     In both cases R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
  */

  dint64_t U[1], V[1], X2[1];
  mul_dint (X2, X, X);       // X2 approximates X^2
  evalPC (U, X2);    // cos2pi(X)
  /* since 0 <= X < 2^-11, we have 0.999 < U <= 1 */
  evalPS (V, X, X2); // sin2pi(X)
  /* since 0 <= X < 2^-11, we have 0 <= V < 0.0005 */
  if (!is_cos)
  {
    // sin2pi(R) ~ sin2pi(i/2^11)*cos2pi(X)+cos2pi(i/2^11)*sin2pi(X)
    mul_dint (U, S+i, U);
    /* since 0 <= S[i] < 0.705 and 0.999 < Uin <= 1, we have
       0 <= U < 0.705 */
    mul_dint (V, C+i, V);
    /* For the error analysis, we distinguish the case i=0.
       For i=0, we have S[i]=0 and C[1]=1, thus V is the value computed
       by evalPS() above, with relative error < 2^-124.648.

       For 1 <= i < 256, analyze_sin_case1(rel=true) from sin.sage gives a
       relative error bound of -122.797 (obtained for i=1).
       In all cases, the relative error for the computation of
       sin2pi(i/2^11)*cos2pi(X)+cos2pi(i/2^11)*sin2pi(X) is bounded by -122.797
       not taking into account the approximation error in R:
       |U - sin2pi(R)| < |U| * 2^-122.797, with U the value computed
       after add_dint (U, U, V) below.

       For the approximation error in R, we have:
       cos(x) = sin2pi (R * (1 + eps))
       R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
       Thus cos(x) = sin2pi(R+R*eps)
                   = sin2pi(R)+R*eps*2*pi*cos2pi(theta), theta in [R,R+R*eps]
       Since 2*pi*R/sin(2*pi*R) < pi/2 for R < 1/4, it follows:
       | cos(x) - sin2pi(R) | < pi/2*R*|sin(2*pi*R)|
       | cos(x) - sin2pi(R) | < 2^-126.018 * |sin2pi(R)|.

       Adding both errors we get:
       | cos(x) - U | < |U| * 2^-122.797 + 2^-126.018 * |sin2pi(R)|
                      < |U| * 2^-122.797 + 2^-126.018 * |U| * (1 + 2^-122.797)
                      < |U| * 2^-122.650.
    */
  }
  else
  {
    // cos2pi(R) ~ cos2pi(i/2^11)*cos2pi(X)-sin2pi(i/2^11)*sin2pi(X)
    mul_dint (U, C+i, U);
    mul_dint (V, S+i, V);
    V->sgn = 1 - V->sgn; // negate V
    /* For 0 <= i < 256, analyze_sin_case2(rel=true) from sin.sage gives a
       relative error bound of -123.540 (obtained for i=0):
       |U - cos2pi(R)| < |U| * 2^-123.540, with U the value computed
       after add_dint (U, U, V) below.

       For the approximation error in R, we have:
       cos(x) = cos2pi (R * (1 + eps))
       R = i/2^11 + X, 0 <= R < 1/4, and |eps| < 2^-126.67.
       Thus cos(x) = cos2pi(R+R*eps)
                   = cos2pi(R)-R*eps*2*pi*sin2pi(theta), theta in [R,R+R*eps]
       Since we have R < 1/4, we have cos2pi(R) >= sqrt(2)/2,
       and it follows:
       | cos(x)/cos2pi(R) - 1 | < 2*pi*R*eps/(sqrt(2)/2)
                                < pi/2*eps/sqrt(2)          [since R < 1/4]
                                < 2^-126.518.
       Adding both errors we get:
       | cos(x) - U | < |U| * 2^-123.540 + 2^-126.518 * |cos2pi(R)|
                      < |U| * 2^-123.540 + 2^-126.518 * |U| * (1 + 2^-123.540)
                      < |U| * 2^-123.367.
    */
  }
  add_dint (U, U, V);
  /* If is_cos=0:
     | cos(x) - U | < |U| * 2^-122.650
     If is_cos=1:
     | cos(x) - U | < |U| * 2^-123.367.
     In all cases the total error is bounded by |U| * 2^-122.650.
     The term |U| * 2^-122.650 contributes to at most 2^(128-122.650) < 41 ulps
     relatively to U->lo.
  */
  uint64_t err = 41;
  uint64_t hi0, hi1, lo0, lo1;
  lo0 = U->lo - err;
  hi0 = U->hi - (lo0 > U->lo);
  lo1 = U->lo + err;
  hi1 = U->hi + (lo1 < U->lo);
  /* check the upper 54 bits are equal */
  if ((hi0 >> 10) != (hi1 >> 10))
    {
      static const double exceptions[][3] = {
        {0x1.8000000000009p-23, 0x1.fffffffffff7p-1, 0x1.b56666666666cp-143},
        {0x1.8000000000024p-22, 0x1.ffffffffffdcp-1, 0x1.b56666666667ep-137},
        {0x1.800000000009p-21,  0x1.ffffffffff7p-1,  0x1.b5666666666c4p-131},
        {0x1.20000000000f3p-20, 0x1.fffffffffebcp-1, 0x1.37642666666fdp-127},
        {0x1.800000000024p-20,  0x1.fffffffffdcp-1,  0x1.b5666666667ddp-125},
      };
      for (int k = 0; k < 5; k++)
        {
          if (__builtin_fabs (x) == exceptions[k][0])
            return exceptions[k][1] + exceptions[k][2];
        }
      __builtin_unreachable ();
    }

  if (neg)
    U->sgn = 1 - U->sgn;

  double y = dint_tod (U);

  return y;
}

void
SECTION
__sincos (double x, double *s, double *c)
{
  b64u64_u t = {.f = x};
  int e = (t.u >> 52) & 0x7ff;

  if (__builtin_expect (e == 0x7ff, 0)) /* NaN, +Inf and -Inf. */
    {
#ifdef CORE_MATH_SUPPORT_ERRNO
      if ((t.u << 1) == 0x7ffull<<53) // Inf
        errno = EDOM;
#endif
      if ((t.u << 1) != (0x7ff8ull<<49))
        __feraiseexcept (FE_INVALID);
      t.u = ~0ull;
      *s = t.f;
      *c = t.f;
      return;
    }

  /* now x is a regular number */

  /* For |x| <= 0x1.6a09e667f3bccp-27, sin(x) round to x and cos(x) rounds to 1
     (to nearest) */
  uint64_t ux = t.u & 0x7fffffffffffffff;
  if (ux <= 0x3e46a09e667f3bccull) // |x| <= 0x1.6a09e667f3bccp-27
  {
    // Taylor expansion of sin(x) is x - x^3/6 around zero
    // for x=-0, fma (x, -0x1p-54, x) returns +0
    *s = (x == 0) ? x : __builtin_fma (x, -0x1p-54, x);
    *c = (x == 0) ? 1.0 : 1.0 - 0x1p-54;

#ifdef CORE_MATH_SUPPORT_ERRNO
    if (x != 0 && (__builtin_fabs (x) < 0x1p-1022 || __builtin_fabs (*s) < 0x1p-1022))
      errno = ERANGE; // underflow
#endif

    return;
  }

  // now |x| > 0x1.6a09e667f3bccp-27
  double h, l, hc, lc, err;
  err = sincos_fast (&h, &l, &hc, &lc, x);
  *s  = h + (l - err);
  double right = h + (l + err);
  *c  = hc + (lc - err);
  double right_c = hc + (lc + err);
  if (__builtin_expect (*s == right && *c == right_c, 1))
    return;

  if (*c == right_c) // fast path succeeded for cos
  {
    *s = sin_accurate (x);
    return;
  }

  // cos_accurate expects x > 0
  *c = cos_accurate ((x > 0) ? x : -x);

  if (*s == right) // fast path succeeded for sin
    return;

  *s = sin_accurate (x);
}
#ifndef __sincos
libm_alias_double (__sincos, sincos)
#endif
