/* Correctly rounded hyperbolic cosine for binary64 values.

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
#include <libm-alias-finite.h>
#include "e_sincosh_data.h"

typedef uint64_t u64;
typedef union {double f; u64 u;} b64u64_u;
static __attribute__((noinline)) double as_cosh_database(double, double);

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

static double __attribute__((cold,noinline)) as_exp_accurate(double x, double t, double th, double tl, double *l){
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

static double __attribute__((noinline)) as_cosh_zero(double x){
  static const double ch[][2] = {
    {0x1p-1, -0x1.c7e8db669f624p-111}, {0x1.5555555555555p-5, 0x1.5555555556135p-59},
    {0x1.6c16c16c16c17p-10, -0x1.f49f4a6e838f2p-65}, {0x1.a01a01a01a01ap-16, 0x1.a4ffbe15316aap-76}};
  static const double cl[] = {0x1.27e4fb7789f5cp-22, 0x1.1eed8eff9089cp-29, 0x1.939749ce13dadp-37, 0x1.ae9891efb6691p-45};
  double x2 = x*x , x2l = __builtin_fma(x, x,-x2);
  double y2 = x2 * (cl[0] + x2 * (cl[1] + x2 * (cl[2] + x2 * (cl[3]))));
  double y1 = polydd(x2, x2l, 4, ch, &y2);
  y1 = muldd(y1, y2, x2, x2l, &y2);
  double y0 = fasttwosum(1.0, y1, &y1);
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
  if(__builtin_expect((t.u&(~0ul>>12))==(~0ul>>12), 0)) return as_cosh_database(x, y0 + y1);
  return y0 + y1;
}

static __attribute__((noinline)) double as_cosh_database(double x, double f){
  static const double db[][3] = {
    {0x1.9a5e3cbe1985ep-4, 0x1.01492f72f984bp+0, -0x1p-107},
    {0x1.52a11832e847dp-3, 0x1.0381e68cac923p+0, 0x1p-104},
    {0x1.bf0305e2c6c37p-3, 0x1.061f4c39e16f2p+0, 0x1p-107},
    {0x1.17326ffc09f68p-2, 0x1.099318a43ac8p+0, 0x1p-104},
    {0x1.3d27bf16d8bdbp-2, 0x1.0c6091056e06ap+0, -0x1p-107},
    {0x1.03923f2b47c07p-1,  0x1.219c1989e3373p+0,-0x1p-54},
    {0x1.a6031cd5f93bap-1, 0x1.5bff041b260fep+0, -0x1p-107},
    {0x1.104b648f113a1p+0, 0x1.9efdca62b700ap+0, -0x1p-109},
    {0x1.1585720f35cd9p+0, 0x1.a5bf3acfde4b2p+0, 0x1p-105},
    {0x1.e9cc7ed2e1a7ep+0,  0x1.bb0ff220d8eb5p+1,-0x1p-53},
    {0x1.43180ea854696p+1, 0x1.91f1122b6b63ap+2, 0x1p-102},
    {0x1.725811dcf6782p+2, 0x1.45ea160ddc71fp+7, -0x1p-100},
    {0x1.5afd56f7d565bp+3, 0x1.8ff8e0ccea7cp+14, 0x1p-90},
    {0x1.759a2ad4c4d56p+3, 0x1.cb62eec26bd78p+15, -0x1p-92},
    {0x1.7fce95ea5c653p+3, 0x1.3bf8009648dcp+16, 0x1p-88},
    {0x1.743d5609348acp+4,  0x1.7a87a8bb7fa28p+32,-0x1p-22},
    {0x1.e07e71bfcf06fp+5, 0x1.91ec4412c344fp+85, 0x1p-24},
    {0x1.6474c604cc0d7p+6, 0x1.7a8f65ad009bdp+127, -0x1p+20},
    {0x1.54cd1fea7663ap+7, 0x1.c90810d354618p+244, 0x1p+135},
    {0x1.2da9e5e6af0bp+8, 0x1.27d6fe867d6f6p+434, 0x1p+329},
    {0x1.d6479eba7c971p+8, 0x1.62a88613629b6p+677, -0x1p+568},
  };
  int a = 0, b = sizeof(db)/sizeof(db[0]) - 1, m = (a + b)/2;
  double ax = __builtin_fabs(x);
  while (a <= b) {
    if (db[m][0] < ax)
      a = m + 1;
    else if (db[m][0] == ax) {
      f = db[m][1] + db[m][2];
      break;
    } else
      b = m - 1;
    m = (a + b)/2;
  }
  return f;
}

double
__ieee754_cosh (double x)
{
  /*
    The function sinh(x) is approximated by a minimax polynomial
    cosh(x)~1+x^2*P(x^2) for |x|<0.125. For other arguments the
    identity cosh(x)=(exp(|x|)+exp(-|x|))/2 is used. For |x|<5 both
    exponents are calculated with slightly higher precision than
    double. For 5<|x|<36.736801 the exp(-|x|) is rather small and is
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
  if(__builtin_expect(aix<0x3fc0000000000000ull, 0)){
    if(__builtin_expect(aix<0x3e50000000000000ull, 0)) return __builtin_fma(ax,0x1p-55,1);
    static const double c[] = {
      0x1p-1, 0x1.555555555554ep-5, 0x1.6c16c16c26737p-10, 0x1.a019ffbbcdbdap-16, 0x1.27ffe2df106cbp-22};
    double x2 = x*x, x4 = x2*x2, p = x2*((c[0] + x2*c[1]) + x4*((c[2] + x2*c[3]) + x4*c[4]));
    double e = x2*(4*0x1p-53), lb = 1 + (p - e), ub = 1 + (p + e);
    if(lb == ub) return lb;
    return as_cosh_zero(x);
  }

  // treat large values apart to avoid a spurious invalid exception
  if (__builtin_expect (aix > 0x408633ce8fb9f87dull, 0)) {
    // |x| > 0x1.633ce8fb9f87dp+9
    if(aix>0x7ff0000000000000ull) return x + x; // nan
    if(aix==0x7ff0000000000000ull) return __builtin_fabs(x); // inf
#ifdef CORE_MATH_SUPPORT_ERRNO
    errno = ERANGE;
#endif
    return 0x1p1023 * 2.0;
  }

  int64_t il = ((uint64_t)jt.u<<14)>>40, jl = -il;
  int64_t i1 = il&0x3f, i0 = (il>>6)&0x3f, ie = il>>12;
  int64_t j1 = jl&0x3f, j0 = (jl>>6)&0x3f, je = jl>>12;
  b64u64_u sp = {.u = (uint64_t)(1022 + ie)<<52},
           sm = {.u = (uint64_t)(1022 + je)<<52};
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
      double e = 0.11e-18*th, lb = rh + (rl - e), ub = rh + (rl + e);
      if(lb == ub) return (lb*sp.f)*2;

      th = as_exp_accurate(ax, t, th, tl, &tl);
      th = fasttwosum(th, tl, &tl);
      b64u64_u uh = {.f = th}, ul = {.f = tl};
      int64_t eh = (uh.u>>52)&0x7ff, el = (ul.u>>52)&0x7ff, ml = (ul.u + 8)&(~0ul>>12);
      th += tl;
      th *= 2;
      th *= sp.f;
      if(ml<=16 || eh-el>103) return as_cosh_database(x, th);
      return th;
    }
    double q0h = T0[j0][1], q1h = T1[j1][1], qh = q0h*q1h;
    th *= sp.f;
    tl *= sp.f;
    qh *= sm.f;
    double pm = mx*((ch[0] + mx*ch[1]) + dx2*(ch[2] + mx*ch[3]));
    double em = qh + qh*pm;
    rh = th;
    rl = (tl + em) + th*pp;

    double e = 0.09e-18*rh, lb = rh + (rl - e), ub = rh + (rl + e);
    if(lb == ub) return lb;

    th = as_exp_accurate( ax, t, th, tl, &tl);
    if(__builtin_expect(aix>0x403f666666666666ull, 0)){
      rh = th + qh; rl = ((th - rh) + qh) + tl;
    } else {
      qh = q0h*q1h;
      double q0l = T0[j0][0], q1l = T1[j1][0];
      double ql = q0h*q1l + q1h*q0l + __builtin_fma(q0h,q1h,-qh);
      qh *= sm.f;
      ql *= sm.f;
      qh = as_exp_accurate(-ax,-t, qh, ql, &ql);
      rh = th + qh; rl = (((th - rh) + qh) + ql) + tl;
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

    rh = fph + fmh;
    rl = ((fph - rh) + fmh) + fml + fpl;
    double e = 0.28e-18*rh, lb = rh + (rl - e), ub = rh + (rl + e);
    if(lb == ub) return lb;
    th = as_exp_accurate( ax, t, th, tl, &tl);
    qh = as_exp_accurate(-ax,-t, qh, ql, &ql);
    rh = th + qh;
    rl = ((th - rh) + qh) + ql + tl;
  }
  rh = fasttwosum(rh, rl, &rl);
  b64u64_u uh = {.f = rh}, ul = {.f = rl};
  int64_t eh = (uh.u>>52)&0x7ff, el = (ul.u>>52)&0x7ff, ml = (ul.u + 8)&(~0ul>>12);
  rh += rl;
  if(__builtin_expect(ml<=16 || eh-el>103,0)) return as_cosh_database(x, rh);
  return rh;
}
libm_alias_finite (__ieee754_cosh, __cosh)
