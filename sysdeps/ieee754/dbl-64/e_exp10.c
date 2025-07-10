/* Correctly rounded 10^x exponential function for binary64 values.

Copyright (c) 2023-2025 Alexei Sibidanov.

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
  double sl, sh = fasttwosum(xh,yh,&sl);
  *e = (xl + yl) + sl;
  return sh;
}

static inline double muldd(double xh, double xl, double ch, double cl, double *l){
  double ahhh = ch*xh;
  *l = cl*xh + ch*xl + __builtin_fma(ch, xh, -ahhh);
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

static inline double as_todenormal(double x){
  b64u64_u ix = {.f = x};
  ix.u &= ~(u64)0>>12;
  // forces the underflow exception
  __feraiseexcept (FE_UNDERFLOW);
  return ix.f;
}

static double __attribute__((noinline)) as_exp10_database(double x, double f){
  static const double db[] = {
    0x1.821e0f2afb97p-11, 0x1.7c3ddd23ac8cap-10, 0x1.a2d7c1699e82dp-10, 0x1.ec65645edc394p-8,
    0x1.90d7373b3a546p-7, 0x1.7e3c84f2cb9b5p-6, 0x1.25765968ecd68p-5, 0x1.9aa6fd4d21a47p-5,
    0x1.e7b525705edefp-5, 0x1.12e02aa997af2p-2, 0x1.c414aa8bd83b1p-2, 0x1.d7d271ab4eeb4p-2,
    0x1.1fe5f30572361p-1, 0x1.522c9f19cc202p-1, 0x1.1daf94cf0bd01p+0, 0x1.75f49c6ad3badp+0,
    0x1.a3c782d4f54fcp+0, 0x1.cc30b915ec8c4p+0, 0x1.ee9674267e65fp+1, 0x1.2d5494eb1dd13p+2,
    0x1.89063309f3004p+4, 0x1.2a59b82b6fc5ep+6, 0x1.cde37694f4d1p+7, -0x1.45ddb10382e3fp-15,
    -0x1.485426a688467p-15, -0x1.6506061aae6f7p-15, -0x1.898a8c3990624p-15, -0x1.17362e953393bp-14,
    -0x1.e40231e216cadp-14, -0x1.7a7f33cc3fd0bp-13, -0x1.63df14c04ab23p-12, -0x1.a1b18d3a28957p-12,
    -0x1.e12494018e44cp-12, -0x1.4c7a2be09b10ep-11, -0x1.de686910f4f52p-11, -0x1.ebb11d32c9493p-10,
    -0x1.f6f96f005fd47p-8, -0x1.b44e17164ce91p-7, -0x1.3b95082297ea7p-6, -0x1.5b25114a07a72p-6,
    -0x1.a9cf11e5adbc5p-4, -0x1.c360cdde773f7p-3, -0x1.56ff305822f26p-2, -0x1.c03419f51b93ep-2,
    -0x1.1416c72a588a6p-1, -0x1.d18176754aac7p-1, -0x1.aa5575135e2d3p+2, -0x1.4cd4af2fca2b4p+4,
    -0x1.da5b10d8689fdp+6};
  b64u64_u ix = {.f = x};
  const b64u64_u *idb = (const b64u64_u*)db;
  int a = 0, b = sizeof(db)/sizeof(db[0]) - 1, m = (a+b)/2;
  while (a <= b) {
    u64 t = idb[m].u;
    if (t < ix.u)
      a = m + 1;
    else if (__builtin_expect(t == ix.u, 0)) {
      static const u64 s2[2] = {0x7eb37ef5ac3fe7c6, 0x3781b19e1};
      const u64 s = 371470981966157;
      b64u64_u d = {.u = ((s>>m)&1)<<63 | 0x3c90000000000000ull}, jf = {.f = f};
      u64 p = s2[m>>5]>>(2*(m&31));
      if(!((jf.u^p)&3)) return jf.f + d.f;
      jf.u -= 1;
      if(!((jf.u^p)&3)) return jf.f + d.f;
      jf.u += 2;
      if(!((jf.u^p)&3)) return jf.f + d.f;
      break;
    } else {
      b = m - 1;
    }
    m = (a + b)/2;
  }
  return f;
}

static double __attribute__((noinline)) as_exp10_accurate(double x){
  static const double c[][2] = {
    {0x1.26bb1bbb55516p+1, -0x1.f48ad494ea102p-53}, {0x1.53524c73cea69p+1, -0x1.e2bfab318d399p-53},
    {0x1.0470591de2ca4p+1, 0x1.81f50779e162bp-53}, {0x1.2bd7609fd98c4p+0, 0x1.31a5cc5d3d313p-54},
    {0x1.1429ffd336aa3p-1, 0x1.910de8c68a0c2p-55}, {0x1.a7ed7086882b4p-3, -0x1.05e703d496537p-57}};
  b64u64_u ix = {.f = x};
  double t = roundeven_finite(0x1.a934f0979a371p+13*x);
  i64 jt = t, i1 = jt&0x3f, i0 = (jt>>6)&0x3f, ie = jt>>12;
  double t0h = T0[i0][1], t0l = T0[i0][0];
  double t1h = T1[i1][1], t1l = T1[i1][0];
  double tl, th = muldd(t0h,t0l, t1h,t1l, &tl);
  const double l0 = 0x1.34413508p-14, l1 = -0x1.f79fef311f12bp-46, l2 = -0x1.ac0b7c917826bp-101;
  double dx = x - l0*t, dxl = l1*t, dxll = l2*t + __builtin_fma(l1,t,-dxl);
  double dxh = dx + dxl; dxl = ((dx - dxh) + dxl) + dxll;
  double fl, fh = opolydd(dxh,dxl, 6,c, &fl);
  fh = muldd(dxh,dxl, fh,fl, &fl);
  if(__builtin_expect(ix.u<0xc0733a7146f72a42ull, 0)){
    if(!(jt&0xfff)){
      fh = fasttwosum(fh, fl, &fl);
      th = fasttwosum(th, fh, &fh);
      fh = fasttwosum(fh, fl, &fl);
      ix.f = fh;
      if(!(ix.u<<12)){
	b64u64_u l = {.f = fl};
	i64 sfh = ((i64)ix.u>>63)^((i64)l.u>>63);
	ix.u += ((i64)1<<51)^sfh;
      }
      fh = th + ix.f;
    } else {
      fh = muldd(fh,fl, th,tl, &fl);
      fh = fastsum(th,tl, fh,fl, &fl);
      fh = fasttwosum(fh, fl, &ix.f);
      if( ((ix.u + 4)&(~(u64)0>>12)) <= 4 || ((ix.u>>52)&0x7ff)<918 ) fh = as_exp10_database(x, fh);
    }
    fh = as_ldexp(fh,ie);
  } else {
    ix.u = (1-ie)<<52;
    fh = muldd(fh,fl, th,tl, &fl);
    fh = fastsum(th,tl, fh,fl, &fl);
    fh = fasttwosum(ix.f, fh, &tl);
    fl += tl;
    fh = as_todenormal(fh + fl);
  }
  return fh;
}

double
__exp10 (double x)
{
  b64u64_u ix = {.f = x};
  u64 aix = ix.u & (~(u64)0>>1);
  if(__builtin_expect(aix>0x40734413509f79feull, 0)){ // |x| > 0x1.34413509f79fep+8
    if(aix>0x7ff0000000000000ull) return x + x; // nan
    if(aix==0x7ff0000000000000ull){
      if(ix.u>>63)
	return 0.0;
      else
	return x;
    }
    if(!(ix.u>>63)) {
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = ERANGE;
#endif
      return 0x1p1023*2.0; // x > 0x1.34413509f79fep+8
    }
    if(aix>0x407439b746e36b52ull) { // x < -0x1.439b746e36b52p+8
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = ERANGE; // underflow
#endif
      return 0x1.8p-1022*0x1p-55;
    }
  }
  // check x integer to avoid a spurious inexact exception
  if(__builtin_expect(!(ix.u<<16), 0)){
    if( (aix>>48) <= 0x4036){
      double kx = roundeven_finite(x);
      if(kx==x){
	i64 k = kx;
	if(k>=0){
	  double r = 1.0;
	  for(i64 i=0; i<k; i++) r *= 10.0;
	  return r;
	}
      }
    }
  }
  /* avoid spurious underflow: for |x| <= 0x1.bcb7b1526e50ep-56,
     exp10(x) rounds to 1 to nearest */
  if (__builtin_expect (aix <= 0x3c7bcb7b1526e50eull, 0))
    return 1.0 + x; // |x| <= 0x1.bcb7b1526e50ep-56
  double t = roundeven_finite(0x1.a934f0979a371p+13*x);
  i64 jt = t, i1 = jt&0x3f, i0 = (jt>>6)&0x3f, ie = jt>>12;
  double t0h = T0[i0][1], t0l = T0[i0][0];
  double t1h = T1[i1][1], t1l = T1[i1][0];
  double tl, th = muldd(t0h,t0l, t1h,t1l, &tl);
  const double l0 = 0x1.34413508p-14, l1 = 0x1.f79fef311f12bp-46;
  double dx = (x - l0*t) - l1*t, dx2 = dx*dx;
  static const double ch[] =
    {0x1.26bb1bbb55516p+1, 0x1.53524c73cea69p+1, 0x1.0470591fd74e1p+1, 0x1.2bd760a1f32a5p+0};
  double p = (ch[0] + dx*ch[1]) + dx2*(ch[2] + dx*ch[3]);
  double fh = th, fx = th*dx, fl = tl + fx*p;
  double eps = 1.63e-19;
  if(__builtin_expect(ix.u<0xc0733a7146f72a42ull, 0)){
    // x > -0x1.33a7146f72a42p+8
    double ub = fh + (fl + eps), lb = fh + (fl - eps);
    if(__builtin_expect( lb != ub, 0)) return as_exp10_accurate(x);
    fh = as_ldexp(fh + fl, ie);
  } else { // x <= -0x1.33a7146f72a42p+8: exp10(x) < 2^-1022
#ifdef CORE_MATH_SUPPORT_ERRNO
    errno = ERANGE; // underflow
#endif
    ix.u = (1-ie)<<52;
    fh = fasttwosum(ix.f, fh, &tl);
    fl += tl;
    double lb = fh + (fl - eps), ub = fh + (fl + eps);
    if(__builtin_expect(lb != ub, 0)) return as_exp10_accurate(x);
    fh = as_todenormal(fh + fl);
  }
  return fh;
}

strong_alias (__exp10, __ieee754_exp10)
libm_alias_finite (__ieee754_exp10, __exp10)
#if LIBM_SVID_COMPAT
versioned_symbol (libm, __exp10, exp10, GLIBC_2_39);
libm_alias_double_other (__exp10, exp10)
#else
libm_alias_double (__exp10, exp10)
#endif
