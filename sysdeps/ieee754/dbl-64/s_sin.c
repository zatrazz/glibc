/* Correctly-rounded sine function for binary64 value.

Copyright (c) 2022-2025 Paul Zimmermann and Tom Hubrecht

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
#include <errno.h>
#include <libm-alias-double.h>
#include "s_sincos.h"
#define CORE_MATH_SUPPORT_ERRNO

/* return the maximal absolute error */
static double
sin_fast (double *h, double *l, double x)
{
  int neg = x < 0, is_sin = 1;
  double absx = neg ? -x : x;

  /* now x > 0x1.7137449123ef6p-26 */
  double err1;
  int i = reduce_fast (h, l, absx, &err1);
  /* err1 is an absolute bound for | i/2^11 + h + l - frac(x/(2pi)) |:
     | i/2^11 + h + l - frac(x/(2pi)) | < err1 */

  // if i >= 2^10: 1/2 <= frac(x/(2pi)) < 1 thus pi <= x <= 2pi
  // we use sin(pi+x) = -sin(x)
  neg = neg ^ (i >> 10);
  i = i & 0x3ff;
  // | i/2^11 + h + l - frac(x/(2pi)) | mod 1/2 < err1

  // now i < 2^10
  // if i >= 2^9: 1/4 <= frac(x/(2pi)) < 1/2 thus pi/2 <= x <= pi
  // we use sin(pi/2+x) = cos(x)
  is_sin = is_sin ^ (i >> 9);
  i = i & 0x1ff;
  // | i/2^11 + h + l - frac(x/(2pi)) | mod 1/4 < err1

  // now 0 <= i < 2^9
  // if i >= 2^8: 1/8 <= frac(x/(2pi)) < 1/4
  // we use sin(pi/2-x) = cos(x)
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
  double err;
  static const double sgn[2] = {1.0, -1.0};
  if (is_sin)
    {
      s_mul (&sh, &sl, sgn[neg] * SC[i][2], sh, sl);
      s_mul (&ch, &cl, sgn[neg] * SC[i][1], ch, cl);
      fast_two_sum (h, l, ch, sh);
      *l += sl + cl;
      /* absolute error bounded by 2^-68.588
         from global_error(is_sin=true,rel=false) in sin.sage:
         | h + l - sin2pi (R) | < 2^-68.588
         thus:
         | h + l - sin |x| | < 2^-68.588 + | sin2pi (R) - sin |x| |
                             < 2^-68.588 + err1 */
      err = 0x1.55p-69; // 2^-66.588 < 0x1.55p-69
    }
  else
    {
      s_mul (&ch, &cl, sgn[neg] * SC[i][2], ch, cl);
      s_mul (&sh, &sl, sgn[neg] * SC[i][1], sh, sl);
      fast_two_sum (h, l, ch, -sh);
      *l += cl - sl;
      /* absolute error bounded by 2^-68.414
         from global_error(is_sin=false,rel=false) in sin.sage:
         | h + l - cos2pi (R) | < 2^-68.414
         thus:
         | h + l - sin |x| | < 2^-68.414 + | cos2pi (R) - sin |x| |
                             < 2^-68.414 + err1 */
      err = 0x1.81p-69; // 2^-68.414 < 0x1.81p-69
    }
  // *h *= sgn[neg];
  // *l *= sgn[neg];
  return err + err1;
}

/* Assume x is a regular number, and |x| > 0x1.7137449123ef6p-26. */
__attribute__((cold))
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
#if 0
      printf ("Rounding test of accurate path failed for sin(%la)\n", x);
      printf ("Please report the above to core-math@inria.fr\n");
      exit (1);
#else
      __builtin_unreachable ();
#endif
    }

  if (neg)
    U->sgn = 1 - U->sgn;

  double y = dint_tod (U);

  return y;
}

#ifndef SECTION
# define SECTION
#endif

#ifndef IN_SINCOS
double
SECTION
__sin (double x)
{
  b64u64_u t = {.f = x};
  int e = (t.u >> 52) & 0x7ff;

  if (__builtin_expect (e == 0x7ff, 0)) /* NaN, +Inf and -Inf. */
    {
#ifdef CORE_MATH_SUPPORT_ERRNO
      if ((t.u << 1) == 0x7ffull<<53){ // Inf
        errno = EDOM;
        return 0.0 / 0.0;
      }
#endif
      if ((t.u << 1) != 0x7ff8ull<<49){
        return 0.0 / 0.0;
      }
      t.u = ~0ull;
      return t.f;
    }

  /* now x is a regular number */

  /* For |x| <= 0x1.7137449123ef6p-26, sin(x) rounds to x (to nearest):
     we can assume x >= 0 without loss of generality since sin(-x) = -sin(x),
     we have x - x^3/6 < sin(x) < x for say 0 < x <= 1 thus
     |sin(x) - x| < x^3/6.
     Write x = c*2^e with 1/2 <= c < 1.
     Then ulp(x)/2 = 2^(e-54), and x^3/6 = c^3/6*2^(3e), thus
     x^3/6 < ulp(x)/2 rewrites as c^3/6*2^(3e) < 2^(e-54),
     or c^3*2^(2e+53) < 3 (1).
     For e <= -26, since c^3 < 1, we have c^3*2^(2e+53) < 2 < 3.
     For e=-25, (1) rewrites 8*c^3 < 3 which yields c <= 0x1.7137449123ef6p-1.
  */
  uint64_t ux = t.u & 0x7fffffffffffffff;
  // 0x3e57137449123ef6 = 0x1.7137449123ef6p-26
  if (ux <= 0x3e57137449123ef6) {
    if (x == 0)
      return x;
    // Taylor expansion of sin(x) is x - x^3/6 around zero
    // for x=-0, fma (x, -0x1p-54, x) returns +0
    /* We have underflow when 0 < |x| < 2^-1022 or when |x| = 2^-1022
       and rounding towards zero. */
    double res = __builtin_fma (x, -0x1p-54, x);
#ifdef CORE_MATH_SUPPORT_ERRNO
    if (__builtin_fabs (x) < 0x1p-1022 || __builtin_fabs (res) < 0x1p-1022)
      errno = ERANGE; // underflow
#endif
    return res;
  }

  double h, l, err;
  err = sin_fast (&h, &l, x);
  double left  = h + (l - err), right = h + (l + err);
  /* With SC[] from ./buildSC 15 we get 1100 failures out of 50000000
     random tests, i.e., about 0.002%. */
  if (__builtin_expect (left == right, 1))
    return left;

  return sin_accurate (x);
}

#ifndef __sin
libm_alias_double (__sin, sin)
#endif

#endif
