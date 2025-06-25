/* Correctly-rounded arc cosine of binary64 value.

Copyright (c) 2024-2025 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/acos/acos.c, revision e9ca2912).

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
#include <stdint.h>
#include <errno.h>
#include <libm-alias-finite.h>
#include "math_config.h"
#include "e_acos_data.h"

typedef uint64_t u64;
typedef int64_t i64;
typedef unsigned short ushort;
typedef union {double f; uint64_t u;} b64u64_u;

static inline double fasttwosum(double x, double y, double *e){
  double s = x + y, z = s - x;
  *e = y - z;
  return s;
}

static inline double twosum(double xh, double ch, double *l){
  double s = xh + ch, d = s - xh;
  *l = (ch - d) + (xh + (d - s));
  return s;
}

static inline double fastsum(double xh, double xl, double yh, double yl, double *e){
  double sl, sh = fasttwosum(xh, yh, &sl);
  *e = (xl + yl) + sl;
  return sh;
}

static inline double fasttwosub(double x, double y, double *e){
  double s = x - y;
  *e = (x - s) - y;
  return s;
}

static inline double sum(double xh, double xl, double ch, double cl, double *l){
  double sl, sh = twosum(xh,ch, &sl);
  *l = (xl + cl) + sl;
  return sh;
}

static inline double muldd(double xh, double xl, double ch, double cl, double *l){
  double ahhh = ch*xh;
  *l = (cl*xh + ch*xl) + __builtin_fma(ch, xh, -ahhh);
  return ahhh;
}

static inline double polydd(double xh, double xl, int n, const double c[][2], double *l){
  int i = n-1;
  double ch = fasttwosum(c[i][0], *l, l), cl = c[i][1] + *l;
  while(--i>=0){
    ch = muldd(xh,xl, ch,cl, &cl);
    ch = fastsum(c[i][0],c[i][1], ch,cl, &cl);
  }
  *l = cl;
  return ch;
}

static double __attribute__((noinline,cold)) as_acos_refine(double, double);

double __ieee754_acos (double x){
  b64u64_u ix = {.f = x};
  u64 ax = ix.u<<1;
  double t,z,zl,jd,f0h,f0l;
  if(ax>0x7fc0000000000000ull){ // |x|>0.5
    static const double off[][2] = {{0,0}, {0x1.921fb54442d18p+1, 0x1.1a62633145c07p-53}};
    i64 k = ix.u>>63;
    f0h = off[k][0];
    f0l = off[k][1];
    if(__builtin_expect(ax>=0x7fe0000000000000ull, 0)){ // |x| >= 1
      if(ax==0x7fe0000000000000ull) return f0h + f0l; // |x| = 1
      if(ax>0xffe0000000000000ull) return x + x; // nan
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = EDOM;
#endif
      return 0./0.; // |x|>1
    }
    // for x>0.5 we use range reduction for double angle formula
    // acos(x) = 2*asin((1-x)/2) and for x<-0.5 acos(x) = pi -
    // 2*asin((1-x)/2)
    t = 2 - 2*__builtin_fabs(x);
    jd = roundeven_finite(t*0x1p5);
    z = __builtin_copysign(__builtin_sqrt(t), x);
    zl = __builtin_fma(z,z,-t)*((-0.5/t)*z);
    t = 0.25*t - jd*0x1p-7;
  } else { // |x|<=0.5
    f0h = 0x1.921fb54442d18p+0;
    f0l = 0x1.1a62633145c07p-54;
    // for |x| <= 0x1.cb3b399d747f2p-55, acos(x) rounds to pi/2 to nearest
    // this avoids a spurious underflow exception with the code below
    if(__builtin_expect(ax <= 0x7919676733ae8fe4, 0)) return f0h + f0l;

    // for |x|<=0.5 we use acos(x) = pi/2 - asin(x) so the argument
    // range for asin is the same for both branches to reuse the lookup
    // tables.
    t = x*x;
    jd = roundeven_finite(t*0x1p7);
    t = __builtin_fma(x,x,-0x1p-7*jd);
    z = -x;
    zl = 0;
  }
  // asin(xh+xl) = (xh + xl)*(cc[j][0] + (cc[j][1] + t*Poly(t, cc[j]+2)))
  // where t = xh^2 - j/128 and j = round(128*xh^2)
  int64_t j = jd;
  const double *c = CC[j];
  double t2 = t*t, d = t*((c[2] + t*c[3]) + t2*((c[4] + t*c[5]) + t2*(c[6] + t*c[7])));
  double fh = c[0], fl = c[1] + d;
  fh = muldd(z,zl, fh,fl, &fl);
  fh = fastsum(f0h,f0l, fh,fl, &fl);
  double eps = __builtin_fabs(z*t)*0x1.8bp-52 + 0x1p-105; // all arguments in [-0x1.1a93e5d11dac2p-1, -0x1.1a86cd0e3b2c2p-1] were checked
  double lb = fh + (fl - eps), ub = fh + (fl + eps);
  if(__builtin_expect(lb!=ub, 0)) return as_acos_refine(x, lb);
  return lb;
}
#ifndef __ieee754_acos
libm_alias_finite (__ieee754_acos, __acos)
#endif

__attribute__((noinline,cold))
static
double as_acos_refine(double x, double phi){
  // Consider x as sin(phi) then cos(phi) is ch + cl = sqrt(1-x^2)
  // Using angle rotation formula bring the argument close to zero
  // where the asin Taylor expansion works well.
  double s2 = x*x, dx2 = __builtin_fma(x,x,-s2);
  double c2l, c2h = fasttwosub(1.0,s2,&c2l);
  c2l -= dx2;
  c2h = fasttwosum(c2h,c2l,&c2l);

  double c2f = __builtin_fma(x,-x,1);
  double ch = __builtin_sqrt(c2f);
  double cl = (c2l - __builtin_fma(ch,ch,-c2f))*((0.5/c2f)*ch);

  int64_t jf = roundeven_finite(__builtin_fabs(phi - 0x1.921fb54442d18p+0) * 0x1.45f306dc9c883p+4);

  // 0 <= jf <= 32
  double Ch = S[32-jf][1], Cl = S[32-jf][0], Sh = S[jf][1], Sl = S[jf][0];

  double ax = __builtin_fabs(x);
  double dsh = ax - Sh, dsl = -Sl;
  double dch = ch - Ch, dcl = cl - Cl;

  double Sc = __builtin_fma(Sh, dch, 0x1.8p-4) - 0x1.8p-4;
  double dSc = __builtin_fma(Sh, dch, -Sc);

  double Cs = __builtin_fma(Ch, dsh, 0x1.8p-4) - 0x1.8p-4;
  double dCs = __builtin_fma(Ch, dsh, -Cs);

  double v = Cs - Sc;
  double dv =  (Ch*dsl + Cl*dsh) - (Sh*dcl + Sl*dch) - (dSc - dCs);
  v = fasttwosum(v,dv,&dv);
  double sgn = __builtin_copysign(1.0, x), jt = 32 - jf*sgn;
  // 0 <= jt <= 64
  double dv2, v2 = muldd(v,dv, v,dv, &dv2);
  v *= -sgn;
  dv *= -sgn;
  double fl = v2*(CT[0] + v2*(CT[1] + v2*CT[2])), fh = polydd(v2,dv2, 5, C, &fl);
  fh = muldd(v,dv, fh,fl, &fl);

  double ph = jt * 0x1.921fb54442dp-5, pl = 0x1.8469898cc518p-53*jt, ps = -0x1.fc8f8cbb5bf6cp-102*jt;
  // since 0 <= jt <= 64, ph and pl are exact
  pl = sum(fh,fl, pl,ps, &ps);
  ph = fasttwosum(ph,pl, &pl);
  pl = fasttwosum(pl,ps, &ps);
  ph = fasttwosum(ph,pl, &pl);
  pl = fasttwosum(pl,ps, &ps);
  b64u64_u t = {.f = pl};
  i64 e = ((t.u>>52)&0x7ff) - 1023;
  e = 52-(107+e);
  e = e<0?0:e;
  e = e>52?52:e;
  u64 m = ((u64)1<<52)-((u64)1<<e);
  e = (e == 0) ? 64 : e;
  if(__builtin_expect(!((t.u+((u64)1<<(e-1)))&m), 0)){
    if(x==-0x1.771164bfd1f84p-3 ) return 0x1.c14601daaf657p+0  - 0x1p-54;
    if(x==-0x1.4510ee8eb4e67p-1 ) return 0x1.211c0e2c2559ep+1  - 0x1p-53;
    if(x==-0x1.011c543f23a17p-2 ) return 0x1.d318c90d9e8b7p+0  - 0x1p-54;
    if(x== 0x1.ffffffffffdc0p-1 ) return 0x1.8000000000024p-22 + 0x1p-76;
    if(x== 0x1.53ea6c7255e88p-4 ) return 0x1.7cdacb6bbe707p+0  + 0x1p-54;
    if(x== 0x1.fd737be914578p-11) return 0x1.91e006d41d8d8p+0  + 0x1.8p-53;
    if(x== 0x1.fffffffffff70p-1 ) return 0x1.8000000000009p-23 + 0x1p-77;
    b64u64_u w = {.f = ps};
    if((w.u^t.u)>>63)
      t.u--;
    else
      t.u++;
    pl = t.f;
  }
  return ph + pl;
}
