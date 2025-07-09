/* Correctly-rounded sine of binary64 value for angles in half-revolutions

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

#include <stdint.h>
#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <libm-alias-double.h>
#include "s_sincospi_data.h"
#define CORE_MATH_SUPPORT_ERRNO

typedef union {double f; uint64_t u;} b64u64_u;

static inline void sincosn(int, double*, double*, double*, double*);
static inline void sincosn2(int, double*, double*, double*, double*);

static inline double fasttwosum(double x, double y, double *e){
  double s = x + y, z = s - x;
  *e = y - z;
  return s;
}

static inline double adddd(double xh, double xl, double ch, double cl, double *l) {
  double s = xh + ch, d = s - xh;
  *l = ((ch - d) + (xh + (d - s))) + (xl + cl);
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

static double as_sinpi_zero(double x){
  double x2 = x*x, dx2 = __builtin_fma(x,x,-x2);
  double x3 = x2*x, dx3 = __builtin_fma(x2,x,-x3) + dx2*x;
  static const double ch[][2] = {
    {-0x1.4abbce625be53p+2, 0x1.05511c68477bep-52}, {0x1.466bc6775aae2p+1, -0x1.6dc0cbefae1dap-54},
    {-0x1.32d2cce62bd86p-1, 0x1.066bd54973829p-55}, {0x1.50783487ee781p-4, 0x1.832989f39a743p-58}};
  static const double cl[3] = {-0x1.e3074fde861fp-8,0x1.e8f4344534da6p-12,-0x1.6f9cd7b8cb9dbp-16};
  double fl = x2*(cl[0]+x2*(cl[1]+x2*(cl[2])));
  double fh = polydd(x2, dx2, 4, ch, &fl);
  fh = muldd(fh, fl, x3, dx3, &fl);
  const double pi0 = 0x1.92p+1, pi1 = 0x1.fb54442d1846ap-11, pi2 = -0x1.d9cceba3f91f2p-65;
  double y0 = pi0*x;
  b64u64_u b = {.f = y0};
  b.u &= ~0ul>>12;
  b.u += (int64_t)85<<51;
  y0 = (y0 + b.f) - b.f;
  double y0l = __builtin_fma(pi0,x,-y0);
  double y1 = pi1*x, y2 = __builtin_fma(pi1,x,-y1) + pi2*x;
  y1 = adddd(y1,y2,y0l,0, &y2);
  y1 = adddd(y1,y2,fh,fl, &y2);
  y0 = fasttwosum(y0,y1, &y1);
  y1 = fasttwosum(y1,y2, &y2);
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

static double as_sinpi_refine(int iq, double z){
  double x = z*0x1p-63, x2 = x*x, dx2 = __builtin_fma(x,x,-x2);
  static const double sh[][2] = {
    {0x1.921fb54442d18p+1, 0x1.1a62633145c06p-53}, {-0x1.4abbce625be53p-22, 0x1.05511cbc65743p-76},
    {0x1.466bc6775aae1p-47, -0x1.9c3c168d990ap-114}};
  static const double ch[][2] = {
    {-0x1.3bd3cc9be45dep-22, -0x1.692b71366cc04p-76}, {0x1.03c1f081b5ac4p-46, -0x1.32b33fda9113cp-100}};
  double sll = -0x1.32d2cc920dcb4p-73*x2;
  double slh = polydd(x2, dx2, 3, sh, &sll);
  slh = mulddd(slh, sll, x*0x1p-12, &sll);
  double cll = x2*(-0x1.55d3c7e3cbff9p-72 + 0x1.e1f50604fa0ffp-99*x2);
  double clh = polydd(x2, dx2, 2, ch, &cll);
  clh = muldd(clh, cll, x2, dx2, &cll);
  double sbh,sbl,cbh,cbl; sincosn2(iq,&sbh,&sbl,&cbh,&cbl);
  double csl, csh = muldd(clh,cll, sbh,sbl, &csl);
  double scl, sch = muldd(slh,sll, cbh,cbl, &scl);
  double tsl, tsh = fasttwosum(sch, csh, &tsl); tsl += csl + scl;
  double tsl2; tsh = fasttwosum(sbh, tsh, &tsl2); tsl = sbl + tsl + tsl2;
  b64u64_u t = {.f = tsl};
  if((t.u|((uint64_t)0xfff<<52)) == ~(uint64_t)0 || (t.u<<12) == 0){
    static const struct {int iq; double x, r, d;} db[] = {
      { 76, -0x1.276b3fef466p-2, 0x1.db8a79a80c3a0p-4,  0x1p-110},
      {108, -0x1.33caea0f24cp-2, 0x1.5146c0bc45bcep-3,  0x1p-109},
      {490, -0x1.8a1e8a3e82cp-1, 0x1.5d6561936b699p-1, -0x1p-55},
      {903, -0x1.bdd02d1ad60p-2, 0x1.f72c906962631p-1,  0x1p-55},
    };
    double sgn = iq>2048?-1:1;
    iq &= 0x7ff;
    for(unsigned i=0; i<sizeof(db)/sizeof(db[0]); i++)
      if((x == db[i].x && iq == db[i].iq) || (x == -db[i].x && iq == 2048-db[i].iq)) return sgn*db[i].r + sgn*db[i].d;
  }
  return tsh + tsl;
}

double __sinpi(double x){
  static const double sn[] = { 0x1.921fb54442d18p-74, -0x1.4abbce625be51p-223, 0x1.466bc6044ba16p-374};
  static const double cn[] = {-0x1.3bd3cc9be45dbp-148, 0x1.03c1f00186416p-298};
  b64u64_u ix = {.f = x};
  uint64_t ax = ix.u&(~(uint64_t)0>>1);
  if(__builtin_expect(ax==0, 0)) return x;
  int32_t e = ax>>52;
  uint64_t m0 = (ix.u&(~(uint64_t)0>>12))|((uint64_t)1<<52);
  int64_t sgn = ix.u; sgn >>= 63;
  int64_t m = ((int64_t)m0^sgn) - sgn;
  int32_t s = 1063 - e;
  if(__builtin_expect(s<0, 0)){
    if(__builtin_expect(e == 0x7ff, 0)){
      if(!(ix.u << 12)){
#ifdef CORE_MATH_SUPPORT_ERRNO
	errno = EDOM;
#endif
	feraiseexcept (FE_INVALID);
	return __builtin_nan("");
      }
      return x + x; // case x=NaN
    }
    s = -s - 1;
    if(s>10) return __builtin_copysign(0.0, x);
    uint64_t iq = (uint64_t)m<<s;
    if(!(iq&2047)) return __builtin_copysign(0.0, x);
    double sh, sl, ch, cl; sincosn((uint64_t)m<<s, &sh, &sl, &ch, &cl);
    return sh + sl;
  }

  if(__builtin_expect(ax<=0x3fa2000000000000ull, 0)){ // |x| <= 0x1.2p-5
    double ph = 0x1.921fb54442d18p+1, pl = 0x1.1a62633145c07p-53;
    double zh, zl;
    if(__builtin_expect(__builtin_fabs(x)<0x1p-54, 0)){
      if(__builtin_expect(__builtin_fabs(x)<0x1p-970, 0)){
#ifdef CORE_MATH_SUPPORT_ERRNO
          /* For all rounding modes, we have underflow (before or after
             rounding) for |x| <= 0x1.45f306dc9c882p-1024. */
        if (__builtin_fabs(x) <= 0x1.45f306dc9c882p-1024)
          errno = ERANGE; // underflow
#endif
	double t = x*0x1p106;
	zh = ph*t; zl = __builtin_fma(ph, t, -zh) + pl*t;
	double r = zh + zl, rs = r*0x1p-106, rt = rs*0x1p106;
	return __builtin_fma((zh - rt) + zl, 0x1p-106, rs);
      }
      zh = ph*x;
      zl = __builtin_fma(ph, x, -zh) + pl*x;
      return zh + zl;
    }
    zh = ph*x;
    zl = __builtin_fma(ph, x, -zh) + pl*x;
    double x2 = x*x, x3 = x2*x, x4 = x2*x2;
    double eps = x*(x2*0x1p-47 + 0x1p-102);
    static const double c[] = {-0x1.4abbce625be51p+2, 0x1.466bc67754b46p+1, -0x1.32d2cc12a51f4p-1, 0x1.5060540058476p-4};
    zl += x3*((c[0] + x2*c[1]) + x4*(c[2] + x2*c[3]));
    double lb = (zl - eps) + zh, ub = (zl + eps) + zh;
    if(lb==ub) return lb;
    return as_sinpi_zero(x);
  }
  
  int32_t si = e-1011;
  if (__builtin_expect(si>=0&&(m0<<(si+1))==0, 0)) {
    // x is integer or half-integer
    if ((m0<<si) == 0)
      return __builtin_copysign(0.0, x); // x is integer
    int t = (m0<<(si-1))>>63;
    // t = 0 if |x| = 1/2 mod 2, t = 1 if |x| = 3/2 mod 2
    return (t == 0) ? __builtin_copysign(1.0, x) : -__builtin_copysign(1.0, x);
  }

  uint64_t iq = (m>>s)&8191;
  iq = (iq + 1)>>1;
  int64_t k = (uint64_t)m<<(e-1000);
  double z = k, z2 = z*z;
  double fs = sn[0] + z2*(sn[1] + z2*sn[2]);
  double fc = cn[0] + z2*cn[1];
  double sh,sl,ch,cl; sincosn(iq,&sh,&sl,&ch,&cl);
  double er = 5.5e-19;
  double r = sl + sh*(z2*fc) + ch*(z*fs);
  double lb = (r - er) + sh, ub = (r + er) + sh;
  if(__builtin_expect(lb == ub,1)) return lb;
  return as_sinpi_refine(iq, z);
}
libm_alias_double (__sinpi, sinpi)

void sincosn(int s, double *sh, double *sl, double *ch, double *cl){
  int j = s&0x3ff, it = -((s>>10)&1);
  j = (~it&j) - ((uint32_t)it<<10) - (it&j);
  int is = j>>5, ic = 0x20 - is, jm = j&0x1f;
  int ss = (s>>11)&1;
  int sc = ((s+1024)>>11)&1;
  
  double sbh = SN[is][0], sbl = SN[is][1];
  double cbh = SN[ic][0], cbl = SN[ic][1];
  double slh = SM[jm][0], sll = SM[jm][1];
  double clh = CM[jm][0], cll = CM[jm][1];

  double sb = sbh + sbl, cb = cbh + cbl;
  double Ch = cbh*clh - sbh*slh, Cl = clh*cbl - slh*sbl + cb*cll - sb*sll;
  double Sh = sbh*clh + cbh*slh, Sl = slh*cbl + clh*sbl + cb*sll + sb*cll;

  double tch = Ch + Cl, tcl = (Ch - tch) + Cl;
  double tsh = Sh + Sl, tsl = (Sh - tsh) + Sl;

  static const double sgn[2] = {0.0, -0.0};
  *ch = __builtin_copysign(1.0, sgn[sc])*tch;
  *cl = __builtin_copysign(1.0, sgn[sc])*tcl;

  *sh = __builtin_copysign(1.0, sgn[ss])*tsh;
  *sl = __builtin_copysign(1.0, sgn[ss])*tsl;
}

void sincosn2(int s, double *sh, double *sl, double *ch, double *cl){
  int j = s&0x3ff, it = -((s>>10)&1);
  j = (~it&j) - ((uint32_t)it<<10) - (it&j);
  int is = j>>5, ic = 0x20 - is, jm = j&0x1f;
  int ss = (s>>11)&1;
  int sc = ((s+1024)>>11)&1;

  double sbh = SN2[is][0], sbl = SN2[is][1];
  double cbh = SN2[ic][0], cbl = SN2[ic][1];
  double slh = SM2[jm][0], sll = SM2[jm][1];
  double clh = CM2[jm][0], cll = CM2[jm][1];

  double ccl, cch = muldd(clh,cll, cbh,cbl, &ccl);
  double ssl, ssh = muldd(slh,sll, sbh,sbl, &ssl);
  double csl, csh = muldd(clh,cll, sbh,sbl, &csl);
  double scl, sch = muldd(slh,sll, cbh,cbl, &scl);

  double tcl, tch = fasttwosum(ssh, cch, &tcl);
  tcl += ccl + ssl;
  double tsl, tsh = fasttwosum(-sch,csh,&tsl); tsl += csl - scl;

  double tcl2; tch = fasttwosum(cbh,-tch, &tcl2); tcl = cbl - tcl + tcl2;
  double tsl2; tsh = fasttwosum(sbh,-tsh, &tsl2); tsl = sbl - tsl + tsl2;
  
  static const double sgn[2] = {0.0, -0.0};
  *ch = __builtin_copysign(1.0, sgn[sc])*tch;
  *cl = __builtin_copysign(1.0, sgn[sc])*tcl;

  *sh = __builtin_copysign(1.0, sgn[ss])*tsh;
  *sl = __builtin_copysign(1.0, sgn[ss])*tsl;
}
