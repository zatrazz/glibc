/* Correctly rounded exponential function for binary64 values.

Copyright (c) 2022-2025 Alexei Sibidanov.

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

#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <math-svid-compat.h>
#include <libm-alias-finite.h>
#include <libm-alias-double.h>
#include "math_config.h"
#include "e_exp_data.h"
#define CORE_MATH_SUPPORT_ERRNO

typedef int64_t i64;
typedef uint64_t u64;
typedef union {double f; u64 u;} b64u64_u;

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

static inline double muldd(double xh, double xl, double ch, double cl, double *l){
  double ahhh = ch*xh;
  *l = (ch*xl + cl*xh) + __builtin_fma(ch, xh, -ahhh);
  return ahhh;
}

static inline double opolydd(double xh, double xl, int n, const double c[][2], double *l){
  int i = n-1;
  double ch = c[i][0], cl = c[i][1];
  while(--i>=0){
    ch = muldd(xh,xl, ch,cl, &cl);
    double th = ch + c[i][0], tl = (c[i][0] - th) + ch;
    ch = th;
    cl += tl + c[i][1];
  }
  *l = cl;
  return ch;
}

static inline double as_ldexp(double x, i64 i){
    b64u64_u ix = {.f = x};
    ix.u += (uint64_t)i<<52;
    return ix.f;
}

// sets the exponent of a binary64 number to 0 (subnormal range)
static inline double as_todenormal(double x){
    b64u64_u ix = {.f = x};
    ix.u &= ~(u64)0>>12;
    // forces the underflow exception
    __feraiseexcept (FE_UNDERFLOW);
    return ix.f;
}

static double __attribute__((noinline)) as_exp_database(double x, double f){
  b64u64_u ix = {.f = x};
  int a = 0, b = __exp_data_db_size - 1, m = (a + b)/2;
  while (a <= b) {
    const double *c = DB;
    if (asuint64 (c[m]) < ix.u) {
      a = m + 1;
    } else if (__builtin_expect(asuint64 (c[m]) == ix.u, 0)) {
      static const u64 s2[2] = {0x57f5fe2e5bde4075ull, 0x3c1f16b8edull};
      const u64 s = 333811522313371;
      b64u64_u jf = {.f = f}, dr = {.u = ((s>>m)<<63)|0x3c90000000000000ull};
      u64 t = (s2[m>>5]>>((m<<1)&63))&3;
      for(i64 k = -1; k<=1; k++){
	b64u64_u r = {.u = jf.u + k};
	if((r.u&3) == t) return r.f + dr.f;
      }
      break;
    } else {
      b = m - 1;
    }
    m = (a + b)>>1;
  }
  return f;
}

static double __attribute__((cold,noinline)) as_exp_accurate(double x){
  static const double ch[][2] =
    {{0x1p+0, 0}, {0x1p-1, 0x1.712f72ecec2cfp-99}, {0x1.5555555555555p-3, 0x1.5555555554d07p-57},
     {0x1.5555555555555p-5, 0x1.55194d28275dap-59}, {0x1.1111111111111p-7, 0x1.12faa0e1c0f7bp-63},
     {0x1.6c16c16da6973p-10, -0x1.4ba45ab25d2a3p-64}, {0x1.a01a019eb7f31p-13, -0x1.9091d845ecd36p-67}};
  b64u64_u ix = {.f = x};
  if(__builtin_expect(((ix.u>>52)&0x7ff)<0x3c9, 0)) return 1 + x;
  const double s = 0x1.71547652b82fep+12;
  double t = roundeven_finite(x*s);
  i64 jt = t, i0 = (jt>>6)&0x3f, i1 = jt&0x3f, ie = jt>>12;
  double t0h = T0[i0][1], t0l = T0[i0][0];
  double t1h = T1[i1][1], t1l = T1[i1][0];
  double tl, th = muldd(t0h,t0l, t1h,t1l, &tl);

  /* Use Cody-Waite argument reduction: since |x| < 745, we have |t| < 2^23,
     thus since l2h is exactly representable on 29 bits, l2h*t is exact. */
  const double l2h = 0x1.62e42ffp-13, l2l = 0x1.718432a1b0e26p-47, l2ll = 0x1.9ff0342542fc3p-102;
  double dx = x - l2h*t, dxl = l2l*t, dxll = l2ll*t + __builtin_fma(l2l,t,-dxl);
  double dxh = dx + dxl; dxl = (dx - dxh) + dxl + dxll;
  double fl, fh = opolydd(dxh,dxl, 7,ch, &fl);
  fh = muldd(dxh,dxl, fh,fl, &fl);
  if(__builtin_expect(ix.u>0xc086232bdd7abcd2ull, 0)){
    // x < -0x1.6232bdd7abcd2p+9
    ix.u = (1-ie)<<52;
    fh = muldd(fh,fl, th,tl, &fl);
    fh = fastsum(th,tl, fh,fl, &fl);
    double e;
    fh = fasttwosum(ix.f, fh, &e);
    fl += e;
    fh = as_todenormal(fh + fl);
  } else {
    if(th == 1.0){
      double e;
      fh = fasttwosum(th,fh, &e);
      fl = fasttwosum(e, fl, &e);
      ix.f = fl;
      if((ix.u&(~(u64)0>>12))==0) {
	b64u64_u v = {.f = e};
	i64 d = ((u64)(((i64)ix.u>>63)^((i64)v.u>>63))<<1) + 1;
	ix.u += d;
	fl = ix.f;
      }
    } else {
      fh = muldd(fh,fl, th,tl, &fl);
      fh = fastsum(th,tl, fh,fl, &fl);
    }
    fh = fasttwosum(fh,fl, &fl);
    ix.f = fl;
    u64 d = (ix.u + 2)&(~(u64)0>>12);
    if(__builtin_expect(d<=2, 0)) fh = as_exp_database(x, fh);
    fh = as_ldexp(fh, ie);
  }
  return fh;
}

#ifndef SECTION
# define SECTION
#endif

double
SECTION
__exp (double x)
{
  b64u64_u ix = {.f = x};
  u64 aix = ix.u & (~(u64)0>>1);
  // exp(x) rounds to 1 to nearest for |x| <= 0x1p-54
  if(__builtin_expect(aix <= 0x3c90000000000000ull, 0)) // |x| <= 0x1p-54
    return 1.0 + x;
  if(__builtin_expect(aix>=0x40862e42fefa39f0ull, 0)){ // |x| >= 0x1.62e42fefa39fp+9
    if(aix>0x7ff0000000000000ull) return x + x; // nan
    if(aix==0x7ff0000000000000ull){ // |x| = inf
      if(ix.u>>63)
	return 0.0; // x = -inf
      else
	return x; // x = inf
    }
    if(!(ix.u>>63)){ // x >= 0x1.62e42fefa39fp+9
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = ERANGE;
#endif
      volatile double z = 0x1p1023;
      return z*z;
    }
    if (aix>=0x40874910d52d3052ull) { // x <= -0x1.74910d52d3052p+9
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = ERANGE; // underflow
#endif
      return 0x1.8p-1022 * 0x1p-55;
    }
  }
  const double s = 0x1.71547652b82fep+12;
  double t = roundeven_finite(x*s);
  i64 jt = t, i0 = (jt>>6)&0x3f, i1 = jt&0x3f, ie = jt>>12;
  double t0h = T0[i0][1], t0l = T0[i0][0];
  double t1h = T1[i1][1], t1l = T1[i1][0];
  double tl, th = muldd(t0h,t0l, t1h,t1l, &tl);
  const double l2h = 0x1.62e42ffp-13, l2l = 0x1.718432a1b0e26p-47;
  /* Use Cody-Waite argument reduction: since |x| < 745, we have |t| < 2^23,
     thus since l2h is exactly representable on 29 bits, l2h*t is exact. */
  double dx = (x - l2h*t) + l2l*t, dx2 = dx*dx;
  static const double ch[] = {0x1p+0, 0x1p-1, 0x1.55555557e54ffp-3, 0x1.55555553a12f4p-5};
  double p = (ch[0] + dx*ch[1]) + dx2*(ch[2] + dx*ch[3]);
  double fh = th, tx = th*dx, fl = tl + tx*p;
  double eps = 1.64e-19;
  if(__builtin_expect(ix.u>0xc086232bdd7abcd2ull, 0)){
    // subnormal case: x < -0x1.6232bdd7abcd2p+9
#ifdef CORE_MATH_SUPPORT_ERRNO
    errno = ERANGE; // underflow
#endif
    ix.u = (1-ie)<<52;
    double e;
    fh = fasttwosum(ix.f, fh, &e);
    fl += e;
    double ub = fh + (fl + eps), lb = fh + (fl - eps);
    if (__builtin_expect(ub != lb, 0)) return as_exp_accurate(x);
    fh = as_todenormal(lb);
  } else {
    double ub = fh + (fl + eps), lb = fh + (fl - eps);
    if(__builtin_expect( ub != lb, 0)) return as_exp_accurate(x);
    fh = as_ldexp(lb, ie);
  }
  return fh;
}
#ifndef __exp
hidden_def (__exp)
strong_alias (__exp, __ieee754_exp)
libm_alias_finite (__ieee754_exp, __exp)
# if LIBM_SVID_COMPAT
versioned_symbol (libm, __exp, exp, GLIBC_2_29);
libm_alias_double_other (__exp, exp)
# else
libm_alias_double (__exp, exp)
# endif
#endif
