/* Correctly rounded exp2 function for binary64 values.

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
  *l = (cl*xh + ch*xl) + __builtin_fma(ch, xh, -ahhh);
  return ahhh;
}

static inline double mulddd(double xh, double ch, double cl, double *l){
  double ahhh = ch*xh;
  *l = cl*xh + __builtin_fma(ch, xh, -ahhh);
  return ahhh;
}

static inline double polydd(double xh, int n, const double c[][2], double *l){
  int i = n-1;
  double ch = c[i][0], cl = c[i][1];
  while(--i>=0){
    ch = mulddd(xh, ch,cl, &cl);
    double th = ch + c[i][0], tl = (c[i][0] - th) + ch;
    ch = th;
    cl += tl + c[i][1];
  }
  *l = cl;
  return ch;
}

static inline double as_ldexp(double x, i64 i){
  b64u64_u ix = {.f = x};
  ix.u += (u64)i<<52;
  return ix.f;
}

// convert x, 2^52 <= x < 2^53 to subnormal range
// if exact <> 0, raises the underflow exception
static inline double as_todenormal(double x, int exact){
  b64u64_u ix = {.f = x};
  ix.u &= ~(u64)0>>12;
  if (!exact)
    // raise the underflow exception
    feraiseexcept (FE_UNDERFLOW);
  return ix.f;
}

static __attribute__((noinline)) double as_exp2_database(double x, double f){
  static const double db[] = {
    0x1.e4596526bf94dp-10, 0x1.e76049073067fp-10, 0x1.755aa6fa428cdp-9, 0x1.79015ce2843d7p-9,
    0x1.f99afefa30d65p-8, 0x1.8d040898b73f5p-6, 0x1.673a7779d5293p-4, 0x1.8859f5e252908p-4,
    0x1.fa18dfad6e466p-4, 0x1.6c4175ea0c6e1p-3, 0x1.926961243babap-3, 0x1.3e34fa6ab969ep-1,
    0x1.b32a6c92d1185p-1, 0x1.9f1a7d355cb4fp+0, -0x1.43c1cea9bd4d9p-13, -0x1.77970470a37edp-13,
    -0x1.7d44c7c8229a6p-13, -0x1.95a914543eab7p-12, -0x1.99be01d01064ap-12, -0x1.13f898b1e4f28p-11,
    -0x1.68e7a49000b1cp-11, -0x1.86d2a6e5e8368p-11, -0x1.120d3bdfb6ed8p-10, -0x1.3ec814d260d02p-10,
    -0x1.47b667916c4b2p-9, -0x1.899e0474ba2d5p-9, -0x1.ba84c6ebfb038p-9, -0x1.111bc29ccdbb1p-8,
    -0x1.1fb57e1996e26p-8, -0x1.72e40977492c3p-8, -0x1.ebf8cf367fcb8p-8, -0x1.07f812303f10ap-7,
    -0x1.234ada2403885p-6, -0x1.35dd739305031p-6, -0x1.526ce079b05a5p-5, -0x1.3ea95a5c16e4ap-4,
    -0x1.33564db4bb9ecp-3, -0x1.d4854d9f87fcap-3, -0x1.fe89353e31cbfp-3, -0x1.83960b2a8d2c4p-2,
    -0x1.e242801b45d0dp-2, -0x1.cef4c143b5adfp-1, -0x1.60e582caa34b1p+0,
  };
  const b64u64_u * idb = (const b64u64_u*)db;
  b64u64_u ix = {.f = x};
  int a = 0, b = sizeof(db)/sizeof(db[0]) - 1, m = (a+b)/2;
  while (a <= b) {
    u64 t = idb[m].u;
    if (t < ix.u)
      a = m + 1;
    else if (t == ix.u) {
      static const u64 s2[2] = {0x3b216fbd5fd7665f, 0x34c797};
      const int64_t k = 8677191773140ul;
      u64 p = (s2[m>>5]>>((m*2)&63))&3;
      b64u64_u jf = {.f = f}, dy = {.u = (u64)(0x3c90|((k>>m)<<15))<<48};
      for(int64_t i=-1;i<=1;i++){
	b64u64_u y = {.u = jf.u + i};
	if( (y.u&3) == p) return y.f + dy.f;
      }
      break;
    } else {
      b = m - 1;
    }
    m = (a + b)/2;
  }
  return f;
}

static double __attribute__((cold,noinline)) as_exp2_accurate(double x){
  b64u64_u ix = {.f = x};
  double sx = 4096.0*x, fx = roundeven_finite(sx), z = sx - fx;
  int64_t k = fx, i1 = k&0x3f, i0 = (k>>6)&0x3f, ie = k>>12;
  double t0h = T0[i0][1], t0l = T0[i0][0];
  double t1h = T1[i1][1], t1l = T1[i1][0];
  double tl, th = muldd(t0h,t0l, t1h,t1l, &tl);
  static const double cd[][2] = {
    {0x1.62e42fefa39efp-13, 0x1.abc9e3b39873ep-68}, {0x1.ebfbdff82c58fp-27, -0x1.5e43a53e4495p-81},
    {0x1.c6b08d704a0cp-41, -0x1.d3a15710d3d83p-95}, {0x1.3b2ab6fba4e77p-55, 0x1.4dd5d2a5e025ap-110},
    {0x1.5d87fe7a66459p-70, -0x1.dc47e47beb9ddp-124}, {0x1.430912f9fb79dp-85, -0x1.4fcd51fcb764p-139}};
  double fl, fh = polydd(z, 6, cd, &fl);
  fh = mulddd(z, fh,fl, &fl);
  if(__builtin_expect(ix.u<=0xc08ff00000000000ull, 1)){ // x >= -1022
    // for -0x1.71547652b82fep-54 <= x <= 0x1.71547652b82fdp-53,
    // exp2(x) round to x to nearest
    if (-0x1.71547652b82fep-54 <= x && x <= 0x1.71547652b82fdp-53)
      return __builtin_fma (x, 0.5, 1.0);
    else if(!(k&0xfff)){ // 4096*x rounds to 4096*integer
      double e;
      fh = fasttwosum(th,fh, &e);
      fl = fasttwosum(e, fl, &e);
      ix.f = fl;
      if((ix.u&(~(u64)0>>12))==0) { // fl is a power of 2
	if((ix.u>>52)&0x7ff){    // |fl| is Inf
	  b64u64_u v = {.f = e};
	  i64 d = ((u64)(((i64)ix.u>>63)^((i64)v.u>>63))<<1) + 1;
	  ix.u += d;
	  fl = ix.f;
	}
      }
    } else {
      fh = muldd(fh,fl, th,tl, &fl);
      fh = fastsum(th,tl, fh,fl, &fl);
    }
    fh = fasttwosum(fh,fl, &fl);
    ix.f = fl;
    u64 d = (ix.u + 2)&(~(u64)0>>12);
    if(__builtin_expect(d<=2, 0)) fh = as_exp2_database(x, fh);
    fh = as_ldexp(fh, ie);
  } else {
    ix.u = ((u64)1-ie)<<52;
    fh = muldd(fh,fl, th,tl, &fl);
    fh = fastsum(th,tl, fh,fl, &fl);
    double e;
    fh = fasttwosum(ix.f, fh, &e);
    fl += e;
    fh = as_todenormal(fh + fl, 0);
  }
  return fh;
}


double
__exp2 (double x)
{
  b64u64_u ix = {.f = x};
  u64 ax = ix.u<<1;
  if(__builtin_expect(ax == 0, 0)) return 1.0;
  if(__builtin_expect(ax >= 0x8120000000000000ull, 0)){ // |x| >= 1024
    if(ax  > 0xffe0000000000000ull) return x + x; // nan
    if(ax == 0xffe0000000000000ull) return (ix.u>>63)?0.0:x; // +/-inf
    if(ix.u>>63){ // x <= -1024
      if(ix.u >= 0xc090cc0000000000ull) { // x <= -1075
#ifdef CORE_MATH_SUPPORT_ERRNO
        errno = ERANGE; // underflow
#endif
	double z = 0x1p-1022;
	return z*z;
      }
    } else { // x >= 1024
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = ERANGE; // overflow
#endif
      return 0x1p1023*x;
    }
  }

  // for |x| <= 0x1.71547652b82fep-54, 2^x rounds to 1 to nearest
  // this avoids a spurious underflow in z*z below
  if (__builtin_expect(ax <= 0x792e2a8eca5705fcull, 0))
    return 1.0 + __builtin_copysign (0x1p-54, x);

  u64 m = ix.u<<12, ex = (ax>>53) - 0x3ff, frac = ex>>63 | m<<(ex&63);
  double sx = 4096.0*x, fx = roundeven_finite(sx), z = sx - fx, z2 = z*z;
  int64_t k = fx, i1 = k&0x3f, i0 = (k>>6)&0x3f, ie = k>>12;
  double t0h = T0[i0][1], t0l = T0[i0][0];
  double t1h = T1[i1][1], t1l = T1[i1][0];
  double tl, th = muldd(t0h,t0l, t1h,t1l, &tl);
  static const double c[] =
    {0x1.62e42fefa39efp-13, 0x1.ebfbdff82c58fp-27, 0x1.c6b08d73b3e01p-41, 0x1.3b2ab6fdda001p-55};
  double tz = th*z, fh = th, fl = tz*((c[0] + z*c[1]) + z2*(c[2] + z*c[3])) + tl;
  double eps = 1.64e-19;
  if(__builtin_expect(ix.u<=0xc08ff00000000000ull, 1)){ // x >= -1022
    // warning: on 32-bit machines, __builtin_expect(frac,1) does not work
    // since only the low 32 bits of frac are taken into account
    if( __builtin_expect(frac != 0, 1)){
      double ub = fh + (fl + eps); fh += fl - eps;
      if(__builtin_expect( ub != fh, 0)) return as_exp2_accurate(x);
    }
    fh = as_ldexp(fh, ie);
  } else { // subnormal case
#ifdef CORE_MATH_SUPPORT_ERRNO
    if (frac != 0)
      errno = ERANGE; // underflow (no underflow when 2^x is exact)
#endif
    ix.u = ((u64)1-ie)<<52;
    double e;
    fh = fasttwosum(ix.f, fh, &e);
    fl += e;
    if(__builtin_expect(frac != 0, 1)){
      double ub = fh + (fl + eps); fh += fl - eps;
      if (__builtin_expect(ub != fh, 0)) return as_exp2_accurate(x);
    }
    // when 2^x is exact, no underflow should be raised
    fh = as_todenormal (fh, frac == 0);
  }
  return fh;
}
#ifndef __exp2
strong_alias (__exp2, __ieee754_exp2)
libm_alias_finite (__ieee754_exp2, __exp2)
# if LIBM_SVID_COMPAT
versioned_symbol (libm, __exp2, exp2, GLIBC_2_29);
libm_alias_double_other (__exp2, exp2)
# else
libm_alias_double (__exp2, exp2)
# endif
#endif
