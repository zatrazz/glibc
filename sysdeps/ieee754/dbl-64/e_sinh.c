/* Correctly rounded hyperbolic sine for binary64 values.

Copyright (c) 2023 Alexei Sibidanov.

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
#include <stdint.h>
#include <errno.h>
#include <libm-alias-finite.h>
#include "e_sincosh_data.h"

#ifndef SECTION
# define SECTION
#endif

typedef uint64_t u64;
typedef union {double f; u64 u;} b64u64_u;

static inline double fasttwosum(double x, double y, double *e){
  double s = x + y, z = s - x;
  *e = y - z;
  return s;
}

static inline double muldd(double xh, double xl, double ch, double cl, double *l){
  double ahlh = ch*xl, alhh = cl*xh, ahhh = ch*xh, ahhl = __builtin_fma(ch, xh, -ahhh);
  ahhl += alhh + ahlh;
  ch = ahhh + ahhl;
  *l = (ahhh - ch) + ahhl;
  return ch;
}

static inline double mulddd(double xh, double xl, double ch, double *l){
  double ahlh = ch*xl, ahhh = ch*xh, ahhl = __builtin_fma(ch, xh, -ahhh);
  ahhl += ahlh;
  ch = ahhh + ahhl;
  *l = (ahhh - ch) + ahhl;
  return ch;
}

static inline double polydd(double xh, double xl, int n, const double c[][2], double *l){
  int i = n-1;
  double ch = c[i][0] + *l, cl = ((c[i][0] - ch) + *l) + c[i][1];
  while(--i>=0){
    ch = muldd(xh, xl, ch, cl, &cl);
    double th = ch + c[i][0], tl = (c[i][0] - th) + ch;
    ch = th;
    cl += tl + c[i][1];
  }
  *l = cl;
  return ch;
}

static double __attribute__((noinline)) as_exp_accurate(double x, double t, double th, double tl, double *l){
  static const double ch[][2] = {
    {0x1p+0, 0x1.6c16bd194535dp-94}, {0x1p-1, -0x1.8259d904fd34fp-93},
    {0x1.5555555555555p-3, 0x1.53e93e9f26e62p-57}};
  const double l2h = 0x1.62e42ffp-13, l2l = 0x1.718432a1b0e26p-47, l2ll = 0x1.9ff0342542fc3p-102;
  double dx = x - l2h*t, dxl = l2l*t, dxll = l2ll*t + __builtin_fma(l2l,t,-dxl);
  double dxh = dx + dxl; dxl = ((dx - dxh) + dxl) + dxll;
  double fl = dxh*(0x1.5555555555555p-5 + dxh *(0x1.11111113e93e9p-7 + dxh *0x1.6c16c169400a7p-10));
  double fh = polydd(dxh,dxl,3,ch, &fl);
  fh = muldd(dxh,dxl,fh,fl,&fl);
  fh = muldd(th,tl,fh,fl,&fl);
  double zh = th + fh, zl = (th-zh) + fh;
  double uh = zh + tl, ul = ((zh-uh) + tl) + zl;
  double vh = uh + fl, vl = ((uh-vh) + fl) + ul;
  *l = vl;
  return vh;
}

static double __attribute__((noinline)) as_sinh_zero(double x){
  static const double ch[][2] = {
    {0x1.5555555555555p-3, 0x1.555555555552fp-57}, {0x1.1111111111111p-7, 0x1.11111115cf00dp-63},
    {0x1.a01a01a01a01ap-13, 0x1.a0011c925b85cp-73}, {0x1.71de3a556c734p-19, -0x1.b4e2835532bcdp-73},
    {0x1.ae64567f54482p-26, -0x1.defcf17a6ab79p-81}};
  double x2 = x*x , x2l = __builtin_fma(x, x,-x2);
  double y2 = x2 * (0x1.6124613aef206p-33 + x2 * (0x1.ae7f36beea815p-41 + x2 * 0x1.95785063cd974p-49));
  double y1 = polydd(x2, x2l, 5, ch, &y2);
  y1 = mulddd(y1, y2, x, &y2);
  y1 = muldd(y1, y2, x2, x2l, &y2);
  double y0 = fasttwosum(x, y1, &y1);
  y1 = fasttwosum(y1, y2, &y2);
  b64u64_u t = {.f = y1};
  if(__builtin_expect(!(t.u&(~0ul>>12)), 0)){
    b64u64_u w = {.f = y2};
    if((w.u^t.u)>>63)
      t.u--;
    else
      t.u++;
    y1 = t.f;
  }
  return y0 + y1;
}

static __attribute__((noinline)) double as_sinh_database(double x, double f){
  static const double db[][3] = {
    {0x1.364303e1ad8f6p-2, 0x1.3b07e0c779ddap-2, -0x1.bcp-106},
    {0x1.4169f234f23b9p-2, 0x1.46b7b3b358f99p-2, -0x1p-56},
    {0x1.616cc75d49226p-2, 0x1.687bd068c1c1ep-2, 0x1.ap-111},
    {0x1.ae3773250e7d2p-2, 0x1.bafc3479fc9ccp-2, -0x1p-105},
    {0x1.d68039861ab53p-2, 0x1.e73b46abb01e1p-2, -0x1.2p-109},
    {0x1.e90f16eb88c09p-2, 0x1.fbdd4a37760b7p-2, -0x1.f8p-108},
    {0x1.a3fc7e4dd47d1p-1, 0x1.d4b21ebf542fp-1, 0x1.ep-107},
    {0x1.aa3b649a96091p-1, 0x1.dd32c5ed1e93p-1, 0x1.8ap-106},
    {0x1.c13876341b62ep-1, 0x1.fd1d7f1c8170cp-1, 0x1.72p-105},
    {0x1.2f5d3b178914ap+0, 0x1.7b8516ffd2406p+0, -0x1.28p-104},
    {0x1.3ffc12b81cbc2p+0, 0x1.9a0ff413a1af3p+0, 0x1.cp-107},
    {0x1.44f65dff00782p+0, 0x1.a38a3c3227609p+0, -0x1p-103},
    {0x1.7346e3c591a14p+0, 0x1.01e9cfa77b855p+1, 0x1.p-102},
    {0x1.b6e2c73f41415p+0, 0x1.57e377b3f0b4bp+1, -0x1p-102},
    {0x1.dc5059d4e507dp+0, 0x1.9168c60ed5256p+1, 0x1.c6p-104},
    {0x1.f737f1e8378c7p+0, 0x1.bffd3f94f40fbp+1, 0x1.a4p-104},
    {0x1.3359640329982p+1, 0x1.5e40df3f985bep+2, 0x1.97p-102},
    {0x1.58a4ff5adac35p+1, 0x1.d671928665bddp+2, 0x1p-102},
    {0x1.8c0a26d055288p+1, 0x1.6056b06a21918p+3, -0x1.bep-102},
    {0x1.bc3c2d0c95f52p+1, 0x1.00fef7383a978p+4, 0x1.61p-100},
    {0x1.0a19aebb51e9p+3, 0x1.fee8f69c4cd25p+10, 0x1.48p-95},
    {0x1.3eb8f61734227p+3, 0x1.4ab1cf45e4e26p+13, 0x1p-90},
    {0x1.43a81752eabe7p+3, 0x1.81d364845ecfap+13, -0x1p-90},
    {0x1.16369cd53bb69p+4, 0x1.0fbc6c02b1c9p+24, -0x1.9p-81},
    {0x1.20e29ea8b51e2p+4, 0x1.08b8abba28abcp+25, 0x1.9bp-79},
    {0x1.a1e4f11b513d7p+4, 0x1.9a65b6c2e2185p+36, -0x1.bcp-70},
    {0x1.c089fcf166171p+4, 0x1.5c452e0e37569p+39, 0x1.4p-69},
    {0x1.e42a98b3a0be5p+4, 0x1.938768ca4f8aap+42, 0x1.6dp-62},
    {0x1.04db52248cbb8p+5, 0x1.0794072349523p+46, 0x1.0e8p-57},
    {0x1.21bc021eeb97ep+5, 0x1.3065064a170fbp+51, 0x1.088p-52},
    {0x1.39fc4d3bb711p+5, 0x1.8a4e90733b95ep+55, 0x1.6ep-50},
    {0x1.3c895d86e96c9p+5, 0x1.0f33837882a6p+56, -0x1.28p-49},
    {0x1.e07e71bfcf06fp+5, 0x1.91ec4412c344fp+85, 0x1p-24},
    {0x1.f7216c4b435c9p+5, 0x1.a97e7be23e65ap+89, -0x1p-15},
    {0x1.6474c604cc0d7p+6, 0x1.7a8f65ad009bdp+127, -0x1.08p+20},
    {0x1.7a60ee15e3e9dp+6, 0x1.62e4dc3bbf53fp+135, 0x1.bp+29},
    {0x1.1f0da93354198p+7, 0x1.0bd73b73fc74cp+206, 0x1.59p+102},
    {0x1.54cd1fea7663ap+7, 0x1.c90810d354618p+244, 0x1.2p+135},
    {0x1.556c678d5e976p+7, 0x1.37e7ac4e7f9b3p+245, 0x1.02p+141},
    {0x1.7945e34b18a9ap+7, 0x1.1b0e4936a8c9bp+271, -0x1.fap+166},
    {0x1.2da9e5e6af0bp+8, 0x1.27d6fe867d6f6p+434, 0x1.0ap+329},
    {0x1.54ceba01331d5p+8, 0x1.9a86785b5ef3ep+490, -0x1.22p+386},
    {0x1.9e7b643238a14p+8, 0x1.f5da7fe652978p+596, 0x1p+493},
    {0x1.c7206c1b753e4p+8, 0x1.8670de0b68cadp+655, -0x1.78p+548},
    {0x1.d6479eba7c971p+8, 0x1.62a88613629b6p+677, -0x1.4p+568},
    {0x1.eb9914d4ac1c8p+8, 0x1.2b67eff65dce8p+708, -0x1.02p+603},
    {0x1.0bc04af1b09f5p+9, 0x1.7b1d97c902985p+771, 0x1.56p+666},
    {0x1.26ee1a46d8c8bp+9, 0x1.fbe20477df4a7p+849, -0x1.55p+745},
    {0x1.4a869881f72acp+9, 0x1.9ea7540a3d1f9p+952, -0x1.2dp+848},
  };
  int a = 0, b = sizeof(db)/sizeof(db[0]) - 1, m = (a + b)/2;
  double ax = __builtin_fabs(x);
  while (a <= b) {
    if (db[m][0] < ax)
      a = m + 1;
    else if (db[m][0] == ax) {
      f = __builtin_copysign(1, x)*db[m][1] + __builtin_copysign(1, x)*db[m][2];
      break;
    } else
      b = m - 1;
    m = (a + b)/2;
  }
  return f;
}

SECTION
double
__ieee754_sinh (double x)
{
  /*
    The function sinh(x) is approximated by a minimax polynomial for
    |x|<0.25. For other arguments the identity
    sinh(x)=(exp(|x|)-exp(-|x|))/2*copysign(1,x) is used. For |x|<5
    both exponents are calculated with slightly higher precision than
    double. For 5<|x|<36.736801 the exp(-|x|) is small and is
    calculated with double precision but exp(|x|) is calculated with
    higher than double precision. For 36.736801<|x|<710.47586
    exp(-|x|) becomes too small and only exp(|x|) is calculated.
   */
  const double s = 0x1.71547652b82fep+12;
  double ax = __builtin_fabs(x), v0 = __builtin_fma(ax, s, 0x1.8000002p+26);
  b64u64_u jt = {.f = v0};
  b64u64_u v = {.f = v0};
  uint64_t tt = ~((1<<26)-1l);
  v.u &= tt;
  double t = v.f - 0x1.8p26;
  b64u64_u ix = {.f = ax};
  u64 aix = ix.u;
  if(__builtin_expect(aix<0x3fd0000000000000ull, 0)){ // |x| < 0x1p-2
    if(__builtin_expect(aix<0x3e57137449123ef7ull, 0)) {
      // |x| < 0x1.7137449123ef7p-26
      /* We have underflow exactly when 0 < |x| < 2^-1022:
         for RNDU, sinh(2^-1022-2^-1074) would round to 2^-1022-2^-1075
         with unbounded exponent range */
#ifdef CORE_MATH_SUPPORT_ERRNO
      if (x != 0 && __builtin_fabs (x) < 0x1p-1022)
        errno = ERANGE; // underflow
#endif
      return __builtin_fma(x,0x1p-55,x);
    }
    static const double c[] =
      {0x1.5555555555555p-3, 0x1.1111111111087p-7, 0x1.a01a01a12e1c3p-13, 0x1.71de2e415aa36p-19, 0x1.aed2bff4269e6p-26};
    double x2 = x*x, x3 = x2*x, x4 = x2*x2, p = x3*((c[0] + x2*c[1]) + x4*((c[2] + x2*c[3]) + x4*c[4]));
    double e = x3*0x1.9p-53, lb = x + (p - e), ub = x + (p + e);
    if(lb == ub) return lb;
    return as_sinh_zero(x);
  }
  if(__builtin_expect(aix>0x408633ce8fb9f87dull, 0)){ // |x| >~ 710.47586
    if(aix>=0x7ff0000000000000ull) return x + x; // nan Inf
#ifdef CORE_MATH_SUPPORT_ERRNO
  errno = ERANGE;
#endif
	return __builtin_copysign(0x1p1023, x)*2.0;
      }
  int64_t il = ((u64)jt.u<<14)>>40, jl = -il;
  int64_t i1 = il&0x3f, i0 = (il>>6)&0x3f, ie = il>>12;
  int64_t j1 = jl&0x3f, j0 = (jl>>6)&0x3f, je = jl>>12;
  b64u64_u sp = {.u = (u64)(1022 + ie)<<52},
           sm = {.u = (u64)(1022 + je)<<52};
  double t0h = T0[i0][1], t0l = T0[i0][0];
  double t1h = T1[i1][1], t1l = T1[i1][0];
  double th = t0h*t1h, tl = t0h*t1l + t1h*t0l + __builtin_fma(t0h,t1h,-th);
  const double l2h = 0x1.62e42ffp-13, l2l = 0x1.718432a1b0e26p-47;
  double dx = (ax - l2h*t) + l2l*t, dx2 = dx*dx, mx = -dx;
  static const double ch[] = {0x1p+0, 0x1p-1, 0x1.5555555aaaaaep-3, 0x1.55555551c98cp-5};
  double pp = dx*((ch[0] + dx*ch[1]) + dx2*(ch[2] + dx*ch[3]));
  double rh, rl;
  if(__builtin_expect(aix>0x4014000000000000ull, 0)){ // |x| > 5
    if(__builtin_expect(aix>0x40425e4f7b2737faull, 0)){ // |x| >~ 36.736801
      sp.u = (1021 + ie)<<52;
      rh = th;
      rl = tl + th*pp;
      rh *= __builtin_copysign(1, x);
      rl *= __builtin_copysign(1, x);
      double e = 0.11e-18*th, lb = rh + (rl - e), ub = rh + (rl + e);
      if(lb == ub) return (lb*sp.f)*2;

      th = as_exp_accurate(ax, t, th, tl, &tl);
      th = fasttwosum(th, tl, &tl);
      th *= __builtin_copysign(1, x);
      tl *= __builtin_copysign(1, x);
      b64u64_u uh = {.f = th}, ul = {.f = tl};
      int64_t eh = (uh.u>>52)&0x7ff, el = (ul.u>>52)&0x7ff, ml = (ul.u + 8)&(~0ul>>12);
      th += tl;
      th *= 2;
      th *= sp.f;
      if(ml<=16 || eh-el>103) return as_sinh_database(x, th);
      return th;
    }
    double q0h = T0[j0][1], q1h = T1[j1][1], qh = q0h*q1h;
    th *= sp.f;
    tl *= sp.f;
    qh *= sm.f;
    double pm = mx*((ch[0] + mx*ch[1]) + dx2*(ch[2] + mx*ch[3]));
    double em = qh + qh*pm;
    rh = th;
    rl = (tl - em) + th*pp;

    rh *= __builtin_copysign(1, x);
    rl *= __builtin_copysign(1, x);
    double e = 0.09e-18*rh, lb = rh + (rl - e), ub = rh + (rl + e);
    if(lb == ub) return lb;

    th = as_exp_accurate( ax, t, th, tl, &tl);
    if(__builtin_expect(aix>0x403f666666666666ull, 0)){
      rh = th - qh; rl = ((th - rh) - qh) + tl;
    } else {
      qh = q0h*q1h;
      double q0l = T0[j0][0], q1l = T1[j1][0];
      double ql = q0h*q1l + q1h*q0l + __builtin_fma(q0h,q1h,-qh);
      qh *= sm.f;
      ql *= sm.f;
      qh = as_exp_accurate(-ax,-t, qh, ql, &ql);
      rh = th - qh; rl = (((th - rh) - qh) - ql) + tl;
    }
  } else {
    double q0h = T0[j0][1], q0l = T0[j0][0];
    double q1h = T1[j1][1], q1l = T1[j1][0];
    double qh = q0h*q1h, ql = q0h*q1l + q1h*q0l + __builtin_fma(q0h,q1h,-qh);
    th *= sp.f;
    tl *= sp.f;
    qh *= sm.f;
    ql *= sm.f;
    double pm = mx*((ch[0] + mx*ch[1]) + dx2*(ch[2] + mx*ch[3]));
    double fph = th, fpl = tl + th*pp;
    double fmh = qh, fml = ql + qh*pm;

    rh = fph - fmh;
    rl = ((fph - rh) - fmh) - fml + fpl;
    rh *= __builtin_copysign(1, x);
    rl *= __builtin_copysign(1, x);
    double e = 0.28e-18*rh, lb = rh + (rl - e), ub = rh + (rl + e);
    if(lb == ub) return lb;
    th = as_exp_accurate( ax, t, th, tl, &tl);
    qh = as_exp_accurate(-ax,-t, qh, ql, &ql);
    rh = th - qh;
    rl = ((th-rh)-qh) - ql + tl;
  }
  rh = fasttwosum(rh, rl, &rl);
  b64u64_u uh = {.f = rh}, ul = {.f = rl};
  int64_t eh = (uh.u>>52)&0x7ff, el = (ul.u>>52)&0x7ff, ml = (ul.u + 8)&(~0ul>>12);
  rh *= __builtin_copysign(1, x);
  rl *= __builtin_copysign(1, x);
  rh += rl;
  if(__builtin_expect(ml<=16 || eh-el>103, 0)) return as_sinh_database(x, rh);
  return rh;
}

#ifndef __ieee754_sinh
libm_alias_finite (__ieee754_sinh, __sinh)
#endif
