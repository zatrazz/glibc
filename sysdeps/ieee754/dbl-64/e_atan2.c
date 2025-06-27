/* Correctly-rounded atan2 function for two binary64 values.

Copyright (c) 2024-2025 Paul Zimmermann and Alexei Sibidanov

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

/* This implementation was possible with the help of Silviu-Ioan Filip,
   who designed the code to generate a rational approximation of atan(z)
   over (0,1). See comments before P[] and Q[] below. */


#include <fenv.h>
#include <fenv_private.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <libm-alias-finite.h>
#include <math-use-builtins-fmax.h>
#include <math-use-builtins-fmin.h>
#include "s_atan2_data.h"
#define CORE_MATH_SUPPORT_ERRNO

#define MASK 0x7fffffffffffffffull // 2^63-1 (mask the sign bit)

// PI_H+PI_L approximates pi with error bounded by 2^-108.041
#define PI_H 0x1.921fb54442d18p+1
#define	PI_L 0x1.1a62633145c07p-53
// PI_OVER2_H+PI_OVER2_L approximates pi/2 with error bounded by 2^-109.041
#define PI_OVER2_H 0x1.921fb54442d18p+0
#define	PI_OVER2_L 0x1.1a62633145c07p-54
// PI_OVER4_H+PI_OVER4_L approximates pi/4 with error bounded by 2^-110.041
#define PI_OVER4_H 0x1.921fb54442d18p-1
#define	PI_OVER4_L 0x1.1a62633145c07p-55

// use a type [29,29] rational approximation of atan(z) for 0 <= z <= 1
static double __attribute__((noinline))
atan2_accurate (double y, double x)
{
  fenv_t env;
  __feholdexcept(&env);
#ifdef FE_OVERFLOW
  int overflow = __fetestexcept (FE_OVERFLOW);
#endif
  double res;
  /* First check when t=y/x is small and exact and x > 0, since for
     |t| <= 0x1.d12ed0af1a27fp-27, atan(t) rounds to t (to nearest). */
  double t = y / x;

#ifdef FE_UNDERFLOW
  /* If t = y/x did underflow for x > 0, then atan(y/x) will underflow
     too, since the Taylor expansion of atan(z) is z - z^3/3 + o(z^3).
     If |t| < 2^-1022 and is exact, then atan(y/x) underflows, and also
     when |t| = 2^-1022, is exact, and rounding is toward zero. */
  int inexact = __fetestexcept (FE_INEXACT);
  double u = __builtin_copysign (1.0, y);
  double v = __builtin_fma (u, -0x1p-54, u);
  // when rounding toward zero, v != u, otherwise v = u
  int underflow = x > 0 && (__fetestexcept (FE_UNDERFLOW) ||
                        (!inexact &&
                         (__builtin_fabs (t) < 0x1p-1022 ||
                          (__builtin_fabs (t) <= 0x1p-1022 && v != u))));
#endif

  /* If t is exact and underflows, then atan(y/x) rounds to t for x > 0,
     to pi for y > 0 and x < 0, and to -pi for x, y < 0. */
  if (t == 0) {
    if (x > 0) {
#if defined FE_UNDERFLOW && defined CORE_MATH_SUPPORT_ERRNO
      if (underflow)
        errno = ERANGE; // underflow
#endif
      __feupdateenv(&env);
      return t;
    }
    res = (y > 0) ? PI_H + PI_L : -PI_H - PI_L;
    goto end;
  }
  double corr = __builtin_fma (t, x, -y);
  if (corr == 0 && x > 0) // t is exact
    if (__builtin_fabs (t) <= 0x1.d12ed0af1a27fp-27)
    {
      // Warning: if y is in the subnormal range, t might differ from y/x
      /* If |y| >= 2^-969, then since t*x has at most 106 significant bits,
         and t*x ~ y, the lower bit of t*x is >= 2^-1074, thus there is no
         underflow in t*x-y. */
      if (__builtin_fabs (y) >= 0x1p-915) {
#if defined FE_UNDERFLOW && defined CORE_MATH_SUPPORT_ERRNO
        if (underflow)
          errno = ERANGE; // underflow
#endif
        __feupdateenv(&env);
        return __builtin_fma (t, -0x1p-54, t);
      }
      /* Now |y| < 2^-969, since x >= 2^-1074, then t <= 2^105, thus we can
         scale y and t by 2^105, which will ensure t*x-y does not underflow. */
      corr = __builtin_fma (t * 0x1p105, x, -y * 0x1p105);
      if (corr == 0) {
        res = __builtin_fma (t, -0x1p-54, t);
#if defined FE_UNDERFLOW && defined CORE_MATH_SUPPORT_ERRNO
        if (underflow)
          errno = ERANGE; // underflow
#endif
        __feupdateenv(&env);
        return res;
      }
    }

  int inv = __builtin_fabs (y) > __builtin_fabs (x);
  tint_t z[1], p[1], q[1];
  if (inv)
    div_tint_d (z, x, y);
  else
    div_tint_d (z, y, x);

  /* When |y/x| < 2^-27, x > 0, atan(y/x) rounds to the same value as y/x
     pertubed by a small amount towards zero (here we subtract 2 to z->l).
     But since the Taylor expansion of atan(t) is t - t^3/3 + O(t^5),
     we have a relative error bounded by t^2/2 for t small enough.
     We thus need |y/x| < 2^-96 so that this error is less than 1 ulp. */
  if (inv == 0 && x > 0 && z->ex <= -96)
    {
      z->l -= 2;
      z->m -= (z->l < 2);
      z->h -= (z->m < 1);
      res = tint_tod (z, 1, y, x);
      goto end;
    }

  // below when we write y/x it should be read x/y when |x/y| < 1
  // |z - y/x| < 2^-185.53 * |z| (relative error from div_tint_d)
  // the rational approximation is only for z > 0, it is not antisymmetric
  int sz = z->sgn;
  z->sgn = 0;
  cp_tint (p, P + 29);
  cp_tint (q, Q + 29);
  for (int i = 28; i >= 0; i--)
  {
    mul_tint (p, p, z);
    mul_tint (q, q, z);
    add_tint (p, p, P + i);
    add_tint (q, q, Q + i);
  }
  // multiply p by z
  mul_tint (p, p, z);
  /* The routine errPsplit(e,13) in atan2.sage gives a relative error bound
     of 2^-184.14 for |p - z*P(z)|, for -11 <= e <= 0, which corresponds
     to 2^-12 <= z <= 1. */
  /* The routine errQsplit(e,12) in atan2.sage gives a relative error bound
     of 2^-184.19 for |q - Q(z)|, for -11 <= e <= 0, which corresponds
     to 2^-12 <= z <= 1. */
  // divide p by q
  div_tint (z, p, q);
  /* The relative error of div_tint() is <= 2^-185.53, thus we have:
     z*P(z)/Q(z) = atan(z) * (1 + eps0) with |eps0| < 3.99613e-59
     z = y/x * (1 + eps1) with |eps1| < 2^-185.53
     p = z*P(z) * (1 + eps2) with |eps2| < 2^-184.14
     q = Q(z) * (1 + eps3)   with |eps3| < 2^-184.19
     newz = p/q * (1 + eps4) with |eps4| < 2^-185.53
     The equality z = y/x * (1 + eps1) gives
     atan(z) = atan(y/x) + eps1*y/x * 1/(1+theta^2) for theta in (z,y/x).
     Thus |atan(z) - atan(y/x)| <= |eps1*y/x| which yields
     |atan(z) - atan(y/x)|/|atan(y/x)| <= |eps1*y/x|/|atan(y/x)|
     Since t/atan(t) is bounded by 1/atan(1) for 0 <= x <= 1, this yields:
     atan(z) = atan(y/x) * (1 + eps5) with |eps5| <= eps1/atan(1) < 2^-185.18.
     In summary we have:
     newz = atan(y/x)*(1+eps0)*(1+eps2)*(1+eps4)*(1+eps5)/(1+eps3)
     thus:
     newz = atan(y/x)*(1+eps6) with |eps6| < 2^-182.63.
     This corresponds to a maximal error of 2^-182.63*2^192 <= 662 ulps.
  */
  uint64_t err = 662; // error bound in case inv=0 and x > 0
  z->sgn = sz; // restore sign
  /* Now z approximates atan(y/x) for inv=0, and atan(x/y) for inv=1,
     with -pi/4 < z < pi/4.
  */
  if (inv)
  {
    // if x/y > 0 thus atan(x/y) > 0 we apply pi/2 - atan(x/y)
    // if x/y < 0 thus atan(x/y) < 0 we apply -pi/2 - atan(x/y)
    if (z->sgn == 0) { // 0 < atan(x/y) < pi/4
      z->sgn = 1;
      add_tint (z, &PI2, z);
      /* Now pi/4 < z < pi/2. The absolute error on z was bounded by
         2^-182.63*pi/4, the error on PI2 is bounded by 2^-197.96, and
         the add_tint() error is bounded by 2 ulp(pi/2) = 2^-190,
         which yields a total error < 2^-182.63*pi/4 + 2^-197.96 + 2^-190
         < 2^-182.967. Relatively to ulp(pi/4) this is less than 524. */
    }
    else // -pi/4 < atan(x/y) < 0
    {
      add_tint (z, &PI2, z);
      z->sgn = 1;
      /* Now -pi/2 < z < -pi/4. The same error analysis as above applies,
         thus we get the same bound of 524 ulps. */
    }
    err = 524;
  }
  // now -pi/2 < z < pi/2
  // if x is negative we go to the opposite quadrant
  if (x < 0) {
    if (z->sgn == 0) { // 1st quadrant -> 3rd quadrant (subtract pi)
      z->sgn = 1;
      add_tint (z, &PI, z);
      z->sgn = 1;
      /* We had 0 < z < pi/2 thus now -pi < z < -pi/2.
         The absolute error on z was bounded by max(2^-182.63*pi/4,2^-182.967)
         = 2^-182.967, that on PI is bounded by 2^-196.96, and the add_tint()
         error is bounded by 2 ulp(pi) = 2^-189, which yields a total error
         < 2^-182.967 + 2^-196.96 + 2^-189 < 2^-182.945.
         Relatively to ulp(pi/2) this is less than 266 ulps. */
    }
    else // 4th quadrant -> 2nd quadrant (add pi)
    {
      add_tint (z, &PI, z);
      /* If inv=0 we had -pi/4 < z < 0 thus now 3pi/4 < z < pi.
         If inv=1 we had -pi/2 < z < -pi/4 thus now pi/2 < z < 3pi/4.
         The same analysis as above applies, and we get the same bound
         of 266 ulps. */
    }
    err = 266;
  }
  res = tint_tod (z, err, y, x);
 end:
#ifdef FE_OVERFLOW
  if (!overflow)
    __feclearexcept (FE_OVERFLOW);
  if (!underflow)
    __feclearexcept (FE_UNDERFLOW);
#ifdef CORE_MATH_SUPPORT_ERRNO
  else
    errno = ERANGE; // underflow
# endif
#endif
  __feupdateenv(&env);
  return res;
}

typedef uint64_t u64;
typedef int64_t i64;
static inline double fasttwosum(double x, double y, double *e){
  double s = x + y, z = s - x;
  *e = y - z;
  return s;
}

static inline double fastsum(double xh, double xl, double yh, double yl, double *e){
  double sl, sh = fasttwosum(xh, yh, &sl);
  *e = (xl + yl) + sl;
  return sh;
}

static double __attribute__((noinline)) as_atan2_special(double y0, double x0){
  d64u64 iy = {.f = y0}, ix = {.f = x0};
  u64 aiy = iy.u<<1, aix = ix.u<<1;

  if (__builtin_expect (aiy >= 0x7ffull<<53 || aix >= 0x7ffull<<53, 0)){ // NaN or Inf
    if (aiy > 0x7ffull<<53 || aix > 0x7ffull<<53)
      // return y0 + x0; // if y or x is sNaN, returns qNaN and raises invalid
      return y0 + x0;
    // Now neither y nor x is NaN, but at least one is +Inf or -Inf
    if (aiy == 0x7ffull<<53 && aix == 0x7ffull<<53){ // both y and x are +/-Inf
      static const double finf[][2] = {{0x1p-55, 0x1.921fb54442d18p-1}, {0x1p-54, 0x1.2d97c7f3321d2p+1}};
      // atan2 (+/-Inf,-Inf) = +/-3pi/4
      // atan2 (+/-Inf,+Inf) = +/-pi/4
      return __builtin_copysign(finf[ix.u>>63][1], y0) + __builtin_copysign(finf[ix.u>>63][0], y0);
    }
    // now only one of y and x is +/-Inf
    if (aix == 0x7ffull<<53) {
      if (x0 < 0)
        return __builtin_copysign (PI_H,y0) + __builtin_copysign (PI_L,y0);
      // atan2(+/-0,x) = +/-0 for x > 0
      // atan2(+/-y,+Inf) = +/-0 for finite y>0
      return __builtin_copysign (0, y0);
    }
    // now y = +/-Inf
    // atan2(+/-Inf,x) = +/-pi/2 for finite x
    return __builtin_copysign (PI_OVER2_H,y0) + __builtin_copysign (PI_OVER2_L, y0);
  }

  if (__builtin_expect (aiy == 0 || aix == 0, 0)){
    if (aiy == 0 && aix == 0){
      if (ix.u == 0) // atan2(+/-0, +0) = +/-0
        return y0;
      // atan2(+/-0, +0) = +/-pi
      return (iy.u == 0) ? PI_H + PI_L : -PI_H - PI_L;
    }
    // only one of y and x is zero
    if (aiy==0){
      // atan2(+/-0,x) = +/-0 for x>0
      if (x0 > 0) return y0/x0;
      // atan2(+/-0,x) = +/-pi for x<0
      return (!(iy.u>>63)) ? PI_H + PI_L : -PI_H - PI_L;
    }
    // now only x is zero
    // atan2(y,+/-0) = -pi/2 for y<0
    // atan2(y,+/-0) = +pi/2 for y>0
    return (y0 > 0) ? PI_OVER2_H + PI_OVER2_L : -PI_OVER2_H - PI_OVER2_L;
  }
  return 0;
}

#ifndef SECTION
# define SECTION
#endif

// avoid spurious underflow in 1/dh
static __always_inline double
rdh_div (double dh)
{
  double rdh;
#ifdef FE_UNDERFLOW
  if (__builtin_expect (__builtin_fabs (dh) <= 0x1p1022, 1))
    rdh = 1/dh;
  else {
    fexcept_t flag;
    __fegetexceptflag (&flag, FE_UNDERFLOW);
    rdh = 1/dh;
    __fesetexceptflag (&flag, FE_UNDERFLOW);
  }
#else
  rdh = 1/dh;
#endif
  return rdh;
}

/* atan2 with max ULP of ~0.524 based on random sampling.  */
double
SECTION
__ieee754_atan2 (double y0, double x0)
{
  static const double asgn[2] = {0.0, -0.0};
  d64u64 iy = {.f = y0}, ix = {.f = x0};
  u64 aiy = iy.u & MASK;
  if(__builtin_expect( aiy==0 || aiy>=0x7ffull<<52, 0)) return as_atan2_special(y0,x0);
  u64 aix = ix.u & MASK;
  if(__builtin_expect( aix==0 || aix>=0x7ffull<<52, 0)) return as_atan2_special(y0,x0);
  double ax = __builtin_fabs(x0), ay = __builtin_fabs(y0);
  double x = USE_FMAX_BUILTIN ? __builtin_fmax(ax, ay) : ax < ay ? ay : ax;
  double y = USE_FMIN_BUILTIN ? __builtin_fmin(ax, ay) : ax > ay ? ay : ax;
  u64 sy = iy.u>>63, sx = ix.u>>63;
  u64 GT = aix<aiy;
  u64 dxy = (aix-aiy)^-GT;
  if(__builtin_expect( dxy>=53ull<<52, 0)) return atan2_accurate(y0,x0);
  d64u64 sgn = {.f = asgn[GT^sx^sy]};
  u64 kw = sx<<2|sy<<1|GT;
  d64u64 jj = {.f = y/x + (2 + 1/128.)};
  i64 jt = ((jj.u>>(52-7))&127);
  double fh = F2[jt][1]*__builtin_copysign(1,sgn.f);
  double fl = F2[jt][0]*__builtin_copysign(1,sgn.f);
  fh += O[kw][0];
  fl += O[kw][1];
  if(__builtin_expect(x<0x1p-968, 0)){x *= 0x1p968; y *= 0x1p968;}
  if(__builtin_expect(x>0x1p1022, 0)){
    if(__builtin_expect(jt != 0,1)){
      x *= 0x1p-1; y *= 0x1p-1;
    }
  }
  double t0 = T2[jt];
  double zn = __builtin_fma(-t0,x,y), zd = __builtin_fma(t0,y,x);
  double z = zn/zd;
  static const double b[] = {-0x1.55555555554d2p-2, 0x1.999999860e1cap-3, -0x1.248ad469844a1p-3};
  double z2 = z*z;
  z *= __builtin_copysign(1,sgn.f);
  double dz = (z*z2)*(b[0] + z2*(b[1] + z2*b[2]));
  // this bound is obtained with 1.1e10 random argument pairs in the roundup mode;
  // the obtained maximal difference is increased by 2.5% for safety
  double eps = __builtin_fabs(z)*0x1.051p-51 + 0x1p-90;
  double rh = fasttwosum(fh, z, &z);
  double rl = (fl + dz) + z;
  double lb = rh + (rl - eps), ub = rh + (rl + eps);
  if(lb!=ub){
    double dh = y*t0, dl = __builtin_fma(y,t0,-dh), e, rdh;
    dh = fasttwosum(x, dh, &e);
    // avoid spurious underflow in 1/dh
    rdh = rdh_div (dh);
    dl += e;
    double nh = x*t0, nl = __builtin_fma(x,t0,-nh);
    double dt = y-nh, y1 = dt+nh;
    if( __builtin_expect(y1 == y, 1)){
      nh = fasttwosum(dt, -nl, &nl);
    } else {
      nh = fasttwosum(dt, (y - y1) - nl, &nl);
    }
    double zh = nh * rdh;
    z2 = zh*zh;
    double zl = rdh * (__builtin_fma(dh, -zh, nh) + (nl - (nh*rdh)*dl));
    static const double b2[] =
      {-0x1.5555555555555p-2, 0x1.999999999755ep-3, -0x1.24924883596f8p-3, 0x1.c6f7d73531bc2p-4};
    zl += zh*z2*((b2[0] + z2*b2[1]) + (z2*z2)*(b2[2] + z2*b2[3]));
    zh *= __builtin_copysign(1,sgn.f);
    zl *= __builtin_copysign(1,sgn.f);
    eps = 0x1.4p-50*(__builtin_fabs(zh)*z2 + 0x1p-51);
    fh = fastsum(fh,fl,zh,zl,&fl);
    lb = fh + (fl - eps);
    ub = fh + (fl + eps);
    if(lb!=ub)
      return atan2_accurate(y0,x0);
  }
  return ub;
}
#ifndef __ieee754_atan2
libm_alias_finite (__ieee754_atan2, __atan2)
#endif
