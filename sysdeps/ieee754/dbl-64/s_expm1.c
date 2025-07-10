/* Correctly rounded e^x-1 function for binary64 values.

Copyright (c) 2024 Alexei Sibidanov.

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
#include <float.h>
#include <math.h>
#include <libm-alias-double.h>
#include "e_exp_data.h"
#include "math_config.h"
#define CORE_MATH_SUPPORT_ERRNO

#ifndef SECTION
# define SECTION
#endif

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

static inline double mulddd(double xh, double xl, double c, double *l){
  double h = c*xh;
  *l = c*xl + __builtin_fma(c, xh, -h);
  return h;
}

static inline double opolydd(double xh, double xl, int n, const double c[][2], double *l){
  int i = n-1;
  double ch = c[i][0], cl = c[i][1];
  while(--i>=0){
    ch = muldd(xh,xl, ch,cl, &cl);
    ch = fastsum(c[i][0],c[i][1], ch,cl, &cl);
  }
  *l = cl;
  return ch;
}

static inline double opolyddd(double x, int n, const double c[][2], double *l){
  int i = n-1;
  double ch = fasttwosum(c[i][0], *l, l), cl = c[i][1] + *l;
  while(--i>=0){
    ch = mulddd(ch,cl, x, &cl);
    ch = fastsum(c[i][0],c[i][1], ch,cl, &cl);
  }
  *l = cl;
  return ch;
}

static inline double as_ldexp(double x, i64 i){
  b64u64_u ix = {.f = x};
  ix.u += (u64)i<<52;
  return ix.f;
}

static const double db[] =
  {0x1.e923c188ea79bp-4, 0x1.1a0408712e00ap-2, 0x1.1c38132777b26p-2, 0x1.27f4980d511ffp-2,
   0x1.8172a0e02f90ep-2, 0x1.8bbe2fb45c151p-2, 0x1.bcab27d05abdep-2, 0x1.005ae04256babp-1,
   0x1.accfbe46b4efp-1, 0x1.d086543694c5ap-1, 0x1.273c188aa7b14p+2, 0x1.83d4bcdebb3f4p+2,
   0x1.08f51434652c3p+4, 0x1.1d5c2daebe367p+4, 0x1.c44ce0d716a1ap+4, 0x1.2ee70220fb1c5p+5,
   0x1.89d56a0c38e6fp+5, 0x1.7a60ee15e3e9dp+6, 0x1.1f0da93354198p+7, 0x1.54cd1fea7663ap+7,
   0x1.556c678d5e976p+7, 0x1.2da9e5e6af0bp+8, 0x1.9e7b643238a14p+8, 0x1.d6479eba7c971p+8,
   0x1.0bc04af1b09f5p+9, -0x1.ab86cb1743b75p-4, -0x1.119aae6072d39p-2, -0x1.175693a03b59p-2,
   -0x1.474d4de7c14bbp-2, -0x1.789d025948efap-2, -0x1.82b5dfaf59b4cp-2, -0x1.9d871e078ebcep-2,
   -0x1.1397add4538acp-1, -0x1.22e24fa3d5cf9p-1, -0x1.dc2b5df1f7d3dp-1, -0x1.0a54d87783d6fp+0,
   -0x1.2a9cad9998262p+0, -0x1.e42a2abb1bf0fp+0, };

static double __attribute__((noinline)) as_expm1_database(double x, double f){
  b64u64_u ix = {.f = x};
  int a = 0, b = sizeof(db)/sizeof(db[0]) - 1, m = (a + b)/2;
  const b64u64_u *c = (const b64u64_u*)db;
  while (a <= b) {
    if (c[m].u < ix.u){
      a = m + 1;
    } else if (__builtin_expect(c[m].u == ix.u, 0)) {
      static const u64 s2[2] = {0x76f58b0d65bd5553ull, 0xc06ull};
      const u64 s = 0x300e81651cull;
      b64u64_u jf = {.f = f}, dr = {.u = ((s>>m)<<63)| (((jf.u>>52)&0x7ff) - 54)<<52};
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

static const double tz[][2] = {
  {-0x1.797d4686c5393p-57, -0x1.c5041854df7d4p-3}, {-0x1.ea1cb9d163339p-55, -0x1.b881a23aebb48p-3},
  {0x1.f483a3e8cd60fp-55, -0x1.abe60e1f21838p-3}, {0x1.dffd920f493dbp-56, -0x1.9f3129931fabp-3},
  {-0x1.51bfdbb129094p-55, -0x1.9262c1c3430ap-3}, {0x1.cd3e5225e2206p-55, -0x1.857aa375db4e4p-3},
  {0x1.e3a6bdaece8f9p-58, -0x1.78789b0a5e0cp-3}, {-0x1.daf2ae0c2d3d4p-55, -0x1.6b5c7478983d8p-3},
  {-0x1.fd36226fadd44p-56, -0x1.5e25fb4fde21p-3}, {0x1.d887cd0341abp-56, -0x1.50d4fab639758p-3},
  {-0x1.676a52a1a618bp-55, -0x1.43693d679612cp-3}, {0x1.9776b420ad283p-56, -0x1.35e28db4ecd9cp-3},
  {0x1.3d5fd7d70a5edp-56, -0x1.2840b5836cf68p-3}, {0x1.a94ad2c8fa0bfp-58, -0x1.1a837e4ba376p-3},
  {0x1.6ad4c353465bp-61, -0x1.0caab118a1278p-3}, {-0x1.8bba170e59b65p-56, -0x1.fd6c2d0e3d91p-4},
  {-0x1.e1e0a76cb0685p-55, -0x1.e14aed893eefp-4}, {0x1.fe131f55e75f8p-55, -0x1.c4f1331d22d4p-4},
  {-0x1.b5beee8bcee31p-55, -0x1.a85e8c62d9c1p-4}, {-0x1.7fe9b02c25e9bp-56, -0x1.8b92870fa2b58p-4},
  {-0x1.32ae7bdaf1116p-55, -0x1.6e8caff341fe8p-4}, {0x1.a6cfe58cbd73bp-56, -0x1.514c92f634788p-4},
  {0x1.8798de3138a56p-57, -0x1.33d1bb17df2e8p-4}, {-0x1.589321a7ef10bp-60, -0x1.161bb26cbb59p-4},
  {-0x1.8d0e700fcfb65p-56, -0x1.f0540438fd5cp-5}, {0x1.473ef07d5dd3bp-55, -0x1.b3f864c08p-5},
  {-0x1.38e62149c16e2p-55, -0x1.77239501304p-5}, {-0x1.08bb6309bd394p-58, -0x1.39d4a1a77e05p-5},
  {-0x1.bad3fd501a227p-55, -0x1.f8152aee945p-6}, {0x1.3d27ac39ed253p-57, -0x1.7b88f290230ep-6},
  {-0x1.b60bbd08aac55p-55, -0x1.fc055004416cp-7}, {-0x1.a00d03b3359dep-59, -0x1.fe0154aaeed8p-8},
  {0x0p+0, 0x0p+0},
  {0x1.861931c15e39bp-55, 0x1.0100ab00222cp-7}, {0x1.7ab864b3e9045p-56, 0x1.0202ad5778e4p-6},
  {0x1.4e5659d75e95bp-56, 0x1.84890d904374p-6}, {0x1.8e0bd083aba81p-56, 0x1.040ac0224fd9p-5},
  {0x1.45cc1cf959b1bp-60, 0x1.465509d383ebp-5}, {-0x1.eb6980ce14da7p-55, 0x1.89246d053d18p-5},
  {0x1.7324137d6c342p-56, 0x1.cc79f4f5613ap-5}, {-0x1.5272ff30eed1bp-59, 0x1.082b577d34ed8p-4},
  {-0x1.1280f19dace1cp-55, 0x1.2a5dd543ccc5p-4}, {-0x1.d550af31c8ec3p-55, 0x1.4cd4fc989cd68p-4},
  {0x1.7923b72aa582dp-55, 0x1.6f9157587069p-4}, {-0x1.76c2e732457f1p-56, 0x1.92937074e0cd8p-4},
  {0x1.81f5c92a5200fp-55, 0x1.b5dbd3f68122p-4}, {0x1.e8ac7a4d3206cp-55, 0x1.d96b0eff0e79p-4},
  {-0x1.12db6f4bbe33bp-56, 0x1.fd41afcba45e8p-4}, {-0x1.8c4a5df1ec7e5p-58, 0x1.10b022db7ae68p-3},
  {-0x1.bd4b1c37ea8a2p-57, 0x1.22e3b09dc54d8p-3}, {0x1.5aeb9860044dp-55, 0x1.353bc9fb00b2p-3},
  {-0x1.4c26602c63fdap-57, 0x1.47b8b853aafecp-3}, {-0x1.7f644c1f9d314p-55, 0x1.5a5ac59b963ccp-3},
  {0x1.f5aa8ec61fc2dp-55, 0x1.6d223c5b10638p-3}, {0x1.7ab912c69ffebp-61, 0x1.800f67b00d7b8p-3},
  {-0x1.b3564bc0ec9cdp-58, 0x1.9322934f54148p-3}, {0x1.6a7062465be33p-55, 0x1.a65c0b85ac1a8p-3},
  {-0x1.85718d2ff1bf4p-55, 0x1.b9bc1d3910094p-3}, {-0x1.045cb0c685e08p-55, 0x1.cd4315e9e0834p-3},
  {-0x1.6e7fb859d5055p-62, 0x1.e0f143b41a554p-3}, {0x1.51bbdee020603p-55, 0x1.f4c6f5508ee5cp-3},
  {0x1.e17611afc42c5p-57, 0x1.04623d0b0f8c8p-2}, {-0x1.1c5b2e8735a43p-56, 0x1.0e7510fd7c564p-2},
  {-0x1.25fe139c4cffdp-55, 0x1.189c1ecaeb084p-2}, {-0x1.89843c4964554p-56, 0x1.22d78f0fa061ap-2},
};

static double __attribute__((noinline)) as_expm1_accurate(double x){
  b64u64_u ix = {.f = x};
  if(__builtin_expect(__builtin_fabs(x)<0.25, 0)){
    static const double cl[] =
      {0x1.93974a8ca5354p-37, 0x1.ae7f3e71e4908p-41, 0x1.ae7f357341648p-45, 0x1.952c7f96664cbp-49,
       0x1.686f8ce633aaep-53, 0x1.2f49b2fbfb5b6p-57};
    static const double ch[][2] = {
      {0x1.5555555555555p-3, 0x1.5555555555554p-57}, {0x1.5555555555555p-5, 0x1.5555555555123p-59},
      {0x1.1111111111111p-7, 0x1.1111111118167p-63}, {0x1.6c16c16c16c17p-10, -0x1.f49f49e220ceap-65},
      {0x1.a01a01a01a01ap-13, 0x1.a019eff6f919cp-73}, {0x1.a01a01a01a01ap-16, 0x1.9fcff48a75b41p-76},
      {0x1.71de3a556c734p-19, -0x1.c14f73758cd7fp-73}, {0x1.27e4fb7789f5cp-22, 0x1.dfce97931018fp-76},
      {0x1.ae64567f544e3p-26, 0x1.c513da9e4c9c5p-80}, {0x1.1eed8eff8d831p-29, 0x1.ca00af84f2b6p-83},
      {0x1.6124613a86e8fp-33, 0x1.f27ac6000898fp-87}
    };

    double fl = x*(cl[0] + x*(cl[1] + x*(cl[2] + x*(cl[3] + x*(cl[4] + x*(cl[5]))))));
    double fh = opolyddd(x, 11,ch, &fl);
    fh = mulddd(fh,fl, x, &fl);
    fh = mulddd(fh,fl, x, &fl);
    fh = mulddd(fh,fl, x, &fl);
    double hx = 0.5*x, x2h = x*hx, x2l = __builtin_fma(x,hx,-x2h);
    fh = fastsum(x2h,x2l, fh,fl, &fl);
    double v2, v0 = fasttwosum(x, fh, &v2), v1 = fasttwosum(v2, fl, &v2);
    v0 = fasttwosum(v0,v1, &v1);
    v1 = fasttwosum(v1,v2, &v2);
    ix.f = v1;
    if((ix.u&(~(u64)0>>12))==0) {
      if(!(ix.u<<1)) return as_expm1_database(x,v0);
      b64u64_u v = {.f = v2};
      i64 d = ((u64)(((i64)ix.u>>63)^((i64)v.u>>63))<<1) + 1;
      ix.u += d;
      v1 = ix.f;
    }
    return v0 + v1;
  } else {
    static const double ch[][2] =
      {{0x1p+0, 0}, {0x1p-1, 0x1.712f72ecec2cfp-99}, {0x1.5555555555555p-3, 0x1.5555555554d07p-57},
       {0x1.5555555555555p-5, 0x1.55194d28275dap-59}, {0x1.1111111111111p-7, 0x1.12faa0e1c0f7bp-63},
       {0x1.6c16c16da6973p-10, -0x1.4ba45ab25d2a3p-64}, {0x1.a01a019eb7f31p-13, -0x1.9091d845ecd36p-67}};
    const double s = 0x1.71547652b82fep+12;
    double t = roundeven_finite(x*s);
    i64 jt = t, i0 = (jt>>6)&0x3f, i1 = jt&0x3f, ie = jt>>12;
    double t0h = T0[i0][1], t0l = T0[i0][0];
    double t1h = T1[i1][1], t1l = T1[i1][0];
    double tl, th = muldd(t0h,t0l, t1h,t1l, &tl);

    const double l2h = 0x1.62e42ffp-13, l2l = 0x1.718432a1b0e26p-47, l2ll = 0x1.9ff0342542fc3p-102;
    double dx = x - l2h*t, dxl = l2l*t, dxll = l2ll*t + __builtin_fma(l2l,t,-dxl);
    double dxh = dx + dxl; dxl = (dx - dxh) + dxl + dxll;
    double fl, fh = opolydd(dxh,dxl, 7,ch, &fl);
    fh = muldd(dxh,dxl, fh,fl, &fl);
    fh = muldd(fh,fl, th,tl, &fl);
    fh = fastsum(th,tl, fh,fl, &fl);
    b64u64_u off = {.u = (u64)(2048+1023-ie)<<52};
    double e;
    if(__builtin_expect(ie<53, 1))
      fh = fasttwosum(off.f, fh, &e);
    else{
      if(ie<104)
	fh = fasttwosum(fh, off.f, &e);
      else
	e = 0;
    }
    fl += e;
    fh = fasttwosum(fh,fl, &fl);
    ix.f = fl;
    u64 d = (ix.u + 8)&(~(u64)0>>12);
    if(__builtin_expect(d<=8, 0)) fh = as_expm1_database(x, fh);
    fh = as_ldexp(fh, ie);
    return fh;
  }
}

SECTION
double
__expm1 (double x)
{
  b64u64_u ix = {.f = x};
  u64 aix = ix.u & (~(u64)0>>1);
  if(__builtin_expect(aix < 0x3fd0000000000000ull, 1)){
    if( __builtin_expect(aix < 0x3ca0000000000000ull, 0)) {
      if( !aix ) return x;
      return __builtin_fma(0x1p-54, __builtin_fabs(x), x);
    }
    double sx = 0x1p7*x, fx = roundeven_finite(sx), z = sx - fx, z2 = z*z;
    i64 i = fx;
    double th = tz[i+32][1], tl = tz[i+32][0];
    static const double c[] =
      {0x1p-7, 0x1p-15, 0x1.55555555551adp-24, 0x1.555555555599cp-33, 0x1.11111ad1ad69dp-42, 0x1.6c16c168b1fb5p-52};
    double fh = z*c[0], fl = z2*((c[1] + z*c[2]) + z2*(c[3] + z*(c[4] + z*c[5])));
    double e0 = 0x1.ap-65, eps = z2*e0 + 0x1p-104;
    double rl, rh = fasttwosum(th,fh,&rl); rl += tl + fl;
    fh = muldd(th,tl, fh,fl, &fl);
    fh = fastsum(rh,rl, fh,fl, &fl);
    double ub = fh + (fl + eps), lb = fh + (fl - eps);
    if(__builtin_expect( ub != lb, 0)) return as_expm1_accurate(x);
    return lb;
  } else {
    if(__builtin_expect(aix>=0x40862e42fefa39f0ull, 0)){
      if(aix>0x7ff0000000000000ull) return x + x; // nan
      if(aix==0x7ff0000000000000ull){
	if(ix.u>>63)
	  return -1.0;
	else
	  return x;
      }
      if(!(ix.u>>63)){
#ifdef CORE_MATH_SUPPORT_ERRNO
  errno = ERANGE;
#endif
	volatile double z = 0x1p1023;
	return z*z;
      }
    }
    if(__builtin_expect(ix.u>=0xc0425e4f7b2737faull, 0)){
      if(ix.u>=0xc042b708872320e2ull) return -1.0 + 0x1p-55;
      return (0x1.25e4f7b2737fap+5 + x + 0x1.8486612173c69p-51)*0x1.71547652b82fep-54 - 0x1.fffffffffffffp-1;
    }

    const double s = 0x1.71547652b82fep+12;
    double t = roundeven_finite(x*s);
    i64 jt = t, i0 = (jt>>6)&0x3f, i1 = jt&0x3f, ie = jt>>12;
    double t0h = T0[i0][1], t0l = T0[i0][0];
    double t1h = T1[i1][1], t1l = T1[i1][0];
    double tl, th = muldd(t0h,t0l, t1h,t1l, &tl);
    const double l2h = 0x1.62e42ffp-13, l2l = 0x1.718432a1b0e26p-47;
    double dx = (x - l2h*t) + l2l*t, dx2 = dx*dx;
    static const double ch[] = {0x1p+0, 0x1p-1, 0x1.55555557e54ffp-3, 0x1.55555553a12f4p-5};
    double p = (ch[0] + dx*ch[1]) + dx2*(ch[2] + dx*ch[3]);
    double fh = th, tx = th*dx, fl = tl + tx*p;
    double eps = 1.64e-19*th;
    b64u64_u off = {.u = (u64)(2048+1023-ie)<<52};
    double e;
    if(__builtin_expect(ie<53, 1)){
      fh = fasttwosum(off.f, fh, &e);
    } else {
      if(ie<75){
	fh = fasttwosum(fh, off.f, &e);
      } else
	e = 0;
    }
    fl += e;
    double ub = fh + (fl + eps), lb = fh + (fl - eps);
    if(__builtin_expect( ub != lb, 0)) return as_expm1_accurate(x);
    return as_ldexp(lb, ie);
  }
}
#ifndef __expm1
libm_alias_double (__expm1, expm1)
#endif
