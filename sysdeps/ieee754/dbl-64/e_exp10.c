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

static const double t0[][2] = {
  {0x0p+0, 0x1p+0}, {-0x1.19083535b085ep-56, 0x1.02c9a3e778061p+0},
  {0x1.d73e2a475b466p-55, 0x1.059b0d3158574p+0}, {0x1.186be4bb285p-57, 0x1.0874518759bc8p+0},
  {0x1.8a62e4adc610ap-54, 0x1.0b5586cf9890fp+0}, {0x1.03a1727c57b52p-59, 0x1.0e3ec32d3d1a2p+0},
  {-0x1.6c51039449b3ap-54, 0x1.11301d0125b51p+0}, {-0x1.32fbf9af1369ep-54, 0x1.1429aaea92dep+0},
  {-0x1.19041b9d78a76p-55, 0x1.172b83c7d517bp+0}, {0x1.e5b4c7b4968e4p-55, 0x1.1a35beb6fcb75p+0},
  {0x1.e016e00a2643cp-54, 0x1.1d4873168b9aap+0}, {0x1.dc775814a8494p-55, 0x1.2063b88628cd6p+0},
  {0x1.9b07eb6c70572p-54, 0x1.2387a6e756238p+0}, {0x1.2bd339940e9dap-55, 0x1.26b4565e27cddp+0},
  {0x1.612e8afad1256p-55, 0x1.29e9df51fdee1p+0}, {0x1.0024754db41d4p-54, 0x1.2d285a6e4030bp+0},
  {0x1.6f46ad23182e4p-55, 0x1.306fe0a31b715p+0}, {0x1.32721843659a6p-54, 0x1.33c08b26416ffp+0},
  {-0x1.63aeabf42eae2p-54, 0x1.371a7373aa9cbp+0}, {-0x1.5e436d661f5e2p-56, 0x1.3a7db34e59ff7p+0},
  {0x1.ada0911f09ebcp-55, 0x1.3dea64c123422p+0}, {-0x1.ef3691c309278p-58, 0x1.4160a21f72e2ap+0},
  {0x1.89b7a04ef80dp-59, 0x1.44e086061892dp+0}, {0x1.3c1a3b69062fp-56, 0x1.486a2b5c13cdp+0},
  {0x1.d4397afec42e2p-56, 0x1.4bfdad5362a27p+0}, {-0x1.4b309d25957e4p-54, 0x1.4f9b2769d2ca7p+0},
  {-0x1.07abe1db13cacp-55, 0x1.5342b569d4f82p+0}, {0x1.9bb2c011d93acp-54, 0x1.56f4736b527dap+0},
  {0x1.6324c054647acp-54, 0x1.5ab07dd485429p+0}, {0x1.ba6f93080e65ep-54, 0x1.5e76f15ad2148p+0},
  {-0x1.383c17e40b496p-54, 0x1.6247eb03a5585p+0}, {-0x1.bb60987591c34p-54, 0x1.6623882552225p+0},
  {-0x1.bdd3413b26456p-54, 0x1.6a09e667f3bcdp+0}, {-0x1.bbe3a683c88aap-57, 0x1.6dfb23c651a2fp+0},
  {-0x1.16e4786887a9ap-55, 0x1.71f75e8ec5f74p+0}, {-0x1.0245957316dd4p-54, 0x1.75feb564267c9p+0},
  {-0x1.41577ee04993p-55, 0x1.7a11473eb0187p+0}, {0x1.05d02ba15797ep-56, 0x1.7e2f336cf4e62p+0},
  {-0x1.d4c1dd41532d8p-54, 0x1.82589994cce13p+0}, {-0x1.fc6f89bd4f6bap-54, 0x1.868d99b4492edp+0},
  {0x1.6e9f156864b26p-54, 0x1.8ace5422aa0dbp+0}, {0x1.5cc13a2e3976cp-55, 0x1.8f1ae99157736p+0},
  {-0x1.75fc781b57ebcp-57, 0x1.93737b0cdc5e5p+0}, {-0x1.d185b7c1b85dp-54, 0x1.97d829fde4e5p+0},
  {0x1.c7c46b071f2bep-56, 0x1.9c49182a3f09p+0}, {-0x1.359495d1cd532p-54, 0x1.a0c667b5de565p+0},
  {-0x1.d2f6edb8d41e2p-54, 0x1.a5503b23e255dp+0}, {0x1.0fac90ef7fd32p-54, 0x1.a9e6b5579fdbfp+0},
  {0x1.7a1cd345dcc82p-54, 0x1.ae89f995ad3adp+0}, {-0x1.2805e3084d708p-57, 0x1.b33a2b84f15fbp+0},
  {-0x1.5584f7e54ac3ap-56, 0x1.b7f76f2fb5e47p+0}, {0x1.23dd07a2d9e84p-55, 0x1.bcc1e904bc1d2p+0},
  {0x1.11065895048dep-55, 0x1.c199bdd85529cp+0}, {0x1.2884dff483cacp-54, 0x1.c67f12e57d14bp+0},
  {0x1.503cbd1e949dcp-56, 0x1.cb720dcef9069p+0}, {-0x1.cbc3743797a9cp-54, 0x1.d072d4a07897cp+0},
  {0x1.2ed02d75b3706p-55, 0x1.d5818dcfba487p+0}, {0x1.c2300696db532p-54, 0x1.da9e603db3285p+0},
  {-0x1.1a5cd4f184b5cp-54, 0x1.dfc97337b9b5fp+0}, {0x1.39e8980a9cc9p-55, 0x1.e502ee78b3ff6p+0},
  {-0x1.e9c23179c2894p-54, 0x1.ea4afa2a490dap+0}, {0x1.dc7f486a4b6bp-54, 0x1.efa1bee615a27p+0},
  {0x1.9d3e12dd8a18ap-54, 0x1.f50765b6e454p+0}, {0x1.74853f3a5931ep-55, 0x1.fa7c1819e90d8p+0}
};

static const double t1[][2] = {
  {0x0p+0, 0x1p+0}, {0x1.ae8e38c59c72ap-54, 0x1.000b175effdc7p+0},
  {-0x1.7b5d0d58ea8f4p-58, 0x1.00162f3904052p+0}, {0x1.4115cb6b16a8ep-54, 0x1.0021478e11ce6p+0},
  {-0x1.d7c96f201bb2ep-55, 0x1.002c605e2e8cfp+0}, {0x1.84711d4c35eap-54, 0x1.003779a95f959p+0},
  {-0x1.0484245243778p-55, 0x1.0042936faa3d8p+0}, {-0x1.4b237da2025fap-54, 0x1.004dadb113dap+0},
  {-0x1.5e00e62d6b30ep-56, 0x1.0058c86da1c0ap+0}, {0x1.a1d6cedbb948p-54, 0x1.0063e3a559473p+0},
  {-0x1.4acf197a00142p-54, 0x1.006eff583fc3dp+0}, {-0x1.eaf2ea42391a6p-57, 0x1.007a1b865a8cap+0},
  {0x1.da93f90835f76p-56, 0x1.0085382faef83p+0}, {-0x1.6a79084ab093cp-55, 0x1.00905554425d4p+0},
  {0x1.86364f8fbe8f8p-54, 0x1.009b72f41a12bp+0}, {-0x1.82e8e14e3110ep-55, 0x1.00a6910f3b6fdp+0},
  {-0x1.4f6b2a7609f72p-55, 0x1.00b1afa5abcbfp+0}, {-0x1.e1a258ea8f71ap-56, 0x1.00bcceb7707ecp+0},
  {0x1.4362ca5bc26f2p-56, 0x1.00c7ee448ee02p+0}, {0x1.095a56c919d02p-54, 0x1.00d30e4d0c483p+0},
  {-0x1.406ac4e81a646p-57, 0x1.00de2ed0ee0f5p+0}, {0x1.b5a6902767e08p-54, 0x1.00e94fd0398ep+0},
  {-0x1.91b206085932p-54, 0x1.00f4714af41d3p+0}, {0x1.427068ab22306p-55, 0x1.00ff93412315cp+0},
  {0x1.c1d0660524e08p-54, 0x1.010ab5b2cbd11p+0}, {-0x1.e7bdfb3204be8p-54, 0x1.0115d89ff3a8bp+0},
  {0x1.843aa8b9cbbc6p-55, 0x1.0120fc089ff63p+0}, {-0x1.34104ee7edae8p-56, 0x1.012c1fecd613bp+0},
  {-0x1.2b6aeb6176892p-56, 0x1.0137444c9b5b5p+0}, {0x1.a8cd33b8a1bb2p-56, 0x1.01426927f5278p+0},
  {0x1.2edc08e5da99ap-56, 0x1.014d8e7ee8d2fp+0}, {0x1.57ba2dc7e0c72p-55, 0x1.0158b4517bb88p+0},
  {0x1.b61299ab8cdb8p-54, 0x1.0163da9fb3335p+0}, {-0x1.90565902c5f44p-54, 0x1.016f0169949edp+0},
  {0x1.70fc41c5c2d54p-55, 0x1.017a28af25567p+0}, {0x1.4b9a6e145d76cp-54, 0x1.018550706ab62p+0},
  {-0x1.008eff5142bfap-56, 0x1.019078ad6a19fp+0}, {-0x1.77669f033c7dep-54, 0x1.019ba16628de2p+0},
  {-0x1.09bb78eeead0ap-54, 0x1.01a6ca9aac5f3p+0}, {0x1.371231477ece6p-54, 0x1.01b1f44af9f9ep+0},
  {0x1.5e7626621eb5ap-56, 0x1.01bd1e77170b4p+0}, {-0x1.bc72b100828a4p-54, 0x1.01c8491f08f08p+0},
  {-0x1.ce39cbbab8bbep-57, 0x1.01d37442d507p+0}, {0x1.16996709da2e2p-55, 0x1.01de9fe280ac8p+0},
  {-0x1.c11f5239bf536p-55, 0x1.01e9cbfe113efp+0}, {0x1.e1d4eb5edc6b4p-55, 0x1.01f4f8958c1c6p+0},
  {-0x1.afb99946ee3fp-54, 0x1.020025a8f6a35p+0}, {-0x1.8f06d8a148a32p-54, 0x1.020b533856324p+0},
  {-0x1.2bf310fc54eb6p-55, 0x1.02168143b0281p+0}, {-0x1.c95a035eb4176p-54, 0x1.0221afcb09e3ep+0},
  {-0x1.491793e46834cp-54, 0x1.022cdece68c4fp+0}, {-0x1.3e8d0d9c4909p-56, 0x1.02380e4dd22adp+0},
  {-0x1.314aa16278aa4p-54, 0x1.02433e494b755p+0}, {0x1.48daf888e965p-55, 0x1.024e6ec0da046p+0},
  {0x1.56dc8046821f4p-55, 0x1.02599fb483385p+0}, {0x1.45b42356b9d46p-54, 0x1.0264d1244c719p+0},
  {-0x1.082ef51b61d7ep-56, 0x1.027003103b10ep+0}, {0x1.2106ed0920a34p-56, 0x1.027b357854772p+0},
  {-0x1.fd4cf26ea5d0ep-54, 0x1.0286685c9e059p+0}, {-0x1.09f8775e78084p-54, 0x1.02919bbd1d1d8p+0},
  {0x1.64cbba902ca28p-58, 0x1.029ccf99d720ap+0}, {0x1.4383ef231d206p-54, 0x1.02a803f2d170dp+0},
  {0x1.4a47a505b3a46p-54, 0x1.02b338c811703p+0}, {0x1.e47120223468p-54, 0x1.02be6e199c811p+0},
};

static double __attribute__((noinline)) as_exp10_accurate(double x){
  static const double c[][2] = {
    {0x1.26bb1bbb55516p+1, -0x1.f48ad494ea102p-53}, {0x1.53524c73cea69p+1, -0x1.e2bfab318d399p-53},
    {0x1.0470591de2ca4p+1, 0x1.81f50779e162bp-53}, {0x1.2bd7609fd98c4p+0, 0x1.31a5cc5d3d313p-54},
    {0x1.1429ffd336aa3p-1, 0x1.910de8c68a0c2p-55}, {0x1.a7ed7086882b4p-3, -0x1.05e703d496537p-57}};
  b64u64_u ix = {.f = x};
  double t = roundeven_finite(0x1.a934f0979a371p+13*x);
  i64 jt = t, i1 = jt&0x3f, i0 = (jt>>6)&0x3f, ie = jt>>12;
  double t0h = t0[i0][1], t0l = t0[i0][0];
  double t1h = t1[i1][1], t1l = t1[i1][0];
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
  double t0h = t0[i0][1], t0l = t0[i0][0];
  double t1h = t1[i1][1], t1l = t1[i1][0];
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
