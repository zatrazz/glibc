/* Correctly-rounded cubic root of binary64 value.

Copyright (c) 2021-2022 Alexei Sibidanov.

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

#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <get-rounding-mode.h>
#include <libm-alias-double.h>

typedef union {double f; uint64_t u;} b64u64_u;

static inline int get_rounding_mode_norm (void)
{
  switch (get_rounding_mode ())
  {
  case FE_TONEAREST:
    return 0;
  case FE_DOWNWARD:
    return 1;
  case FE_UPWARD:
    return 2;
  }
  // case FE_TOWARDZERO:
  return 3;
}

double
__cbrt (double x)
{
  static const double escale[3] = {1.0, 0x1.428a2f98d728bp+0/* 2^(1/3) */, 0x1.965fea53d6e3dp+0/* 2^(2/3) */};
  /* the polynomial c0+c1*x+c2*x^2+c3*x^3 approximates x^(1/3) on [1,2]
     with maximal error < 9.2e-5 (attained at x=2) */
  static const double c[] = {0x1.1b0babccfef9cp-1, 0x1.2c9a3e94d1da5p-1, -0x1.4dc30b1a1ddbap-3, 0x1.7a8d3e4ec9b07p-6};
  const double u0 = 0x1.5555555555555p-2, u1 = 0x1.c71c71c71c71cp-3;
  static const double rsc[] = { 1, -1, 0.5, -0.5, 0.25, -0.25};
  static const double off[] = {0x1p-53, 0, 0, 0};
  unsigned int rm = get_rounding_mode_norm ();
  /* rm=0 for rounding to nearest, and other values for directed roundings */
  b64u64_u cvt0 = {.f = x};
  uint64_t hx = cvt0.u, mant = hx&((~(uint64_t)0)>>12), sign = hx>>63;
  unsigned e = (hx>>52)&0x7ff;
  if(__builtin_expect(((e+1)&0x7ff)<2, 0)){
    uint64_t ix = hx&((~(uint64_t)0)>>1);
    if(e==0x7ff||ix==0) return x + x; /* 0, inf, nan: we return x + x instead of simply x,
                                         to that for x a signaling NaN, it correctly triggers
                                         the invalid exception. */
    /* use __builtin_clzll otherwise ix might be truncated to 32 bits
       on 32-bit processors */
    int nz = __builtin_clzll(ix) - 11;  /* subnormal */
    mant <<= nz;
    mant &= (~((uint64_t)0))>>12;
    e -= nz - 1;
  }
  e += 3072;
  b64u64_u cvt1 = {.u = mant|((uint64_t)0x3ff<<52)}, cvt5 = {.u = cvt1.u};
  unsigned et = e/3, it = e%3;
  /* 2^(3k+it) <= x < 2^(3k+it+1), with 0 <= it <= 3 */
  cvt5.u += (int64_t)it<<52;
  cvt5.u |= sign<<63;
  double zz = cvt5.f;
  /* cbrt(x) = cbrt(zz)*2^(et-1365) where 1 <= zz < 8 */
  uint64_t isc = ((const uint64_t*)escale)[it];
  isc |= sign<<63;
  b64u64_u cvt2 = {.u = isc};
  double z = cvt1.f;
  /* cbrt(zz) = cbrt(z)*isc, where isc encodes 1, 2^(1/3) or 2^(2/3),
     and 1 <= z < 2 */
  double r = 1/z, rr = r*rsc[it<<1|sign], z2 = z*z;
  double c0 = c[0] + z*c[1], c2 = c[2] + z*c[3];
  double y = c0 + z2*c2, y2 = y*y;
  /* y is an approximation of z^(1/3) */
  double h = y2*(y*r) - 1;
  /* h determines the error between y and z^(1/3) */
  y -= (h*y)*(u0 - u1*h);
  /* The correction y -= (h*y)*(u0 - u1*h) corresponds to a cubic variant
     of Newton's method, with the function f(y) = 1-z/y^3. */
  y *= cvt2.f;
  /* Now y is an approximation of zz^(1/3),
     and rr an approximation of 1/zz. We now perform another iteration of
     Newton-Raphson, this time with a linear approximation only. */
  y2 = y*y; double y2l = __builtin_fma(y,y,-y2);
  /* y2 + y2l = y^2 exactly */
  double y3 = y2*y, y3l = __builtin_fma(y,y2,-y3) + y*y2l;
  /* y3 + y3l approximates y^3 with about 106 bits of accuracy */
  h = ((y3 - zz) + y3l)*rr;
  double dy = h*(y*u0);
  /* the approximation of zz^(1/3) is y - dy */
  double y1 = y - dy;
  dy = (y - y1) - dy;
  /* the approximation of zz^(1/3) is now y1 + dy, where |dy| < 1/2 ulp(y)
     (for rounding to nearest) */
  double ady = __builtin_fabs(dy);
  /* For directed roundings, ady0 is tiny when dy is tiny, or ady0 is near
     from ulp(1);
     for rounding to nearest, ady0 is tiny when dy is near from 1/2 ulp(1),
     or from 3/2 ulp(1). */
  double ady0 = __builtin_fabs(ady - off[rm]);
  double ady1 = __builtin_fabs(ady - (0x1p-52+off[rm]));
  if(__builtin_expect(ady0<0x1p-75 || ady1<0x1p-75, 0)){
    y2 = y1*y1; y2l = __builtin_fma(y1,y1,-y2);
    y3 = y2*y1; y3l = __builtin_fma(y1,y2,-y3) + y1*y2l;
    h = ((y3 - zz) + y3l)*rr;
    dy = h*(y1*u0);
    y = y1 - dy;
    dy = (y1 - y) - dy;
    y1 = y;
    ady = __builtin_fabs(dy);
    ady0 = __builtin_fabs(ady - off[rm]);
    ady1 = __builtin_fabs(ady - (0x1p-52+off[rm]));
    if(__builtin_expect(ady0<0x1p-98 || ady1<0x1p-98, 0)){
      double azz = __builtin_fabs(zz);
      if(azz == 0x1.9b78223aa307cp+1) // ~ 0x1.79d15d0e8d59b80000000000000ffc3dp+0
	y1 = __builtin_copysign(0x1.79d15d0e8d59cp+0, zz);
      if(azz == 0x1.a202bfc89ddffp+2) // ~ 0x1.de87aa837820e80000000000001c0f08p+0
	y1 = __builtin_copysign(0x1.de87aa837820fp+0, zz);
      if(rm>0){
	static const double wlist[][2] = {
	  {0x1.3a9ccd7f022dbp+0, 0x1.1236160ba9b93p+0},// ~ 0x1.1236160ba9b930000000000001e7e8fap+0
	  {0x1.7845d2faac6fep+0, 0x1.23115e657e49cp+0},// ~ 0x1.23115e657e49c0000000000001d7a799p+0
	  {0x1.d1ef81cbbbe71p+0, 0x1.388fb44cdcf5ap+0},// ~ 0x1.388fb44cdcf5a0000000000002202c55p+0
	  {0x1.0a2014f62987cp+1, 0x1.46bcbf47dc1e8p+0},// ~ 0x1.46bcbf47dc1e8000000000000303aa2dp+0
	  {0x1.fe18a044a5501p+1, 0x1.95decfec9c904p+0},// ~ 0x1.95decfec9c9040000000000000159e8ep+0
	  {0x1.a6bb8c803147bp+2, 0x1.e05335a6401dep+0},// ~ 0x1.e05335a6401de00000000000027ca017p+0
	  {0x1.ac8538a031cbdp+2, 0x1.e281d87098de8p+0},// ~ 0x1.e281d87098de80000000000000ee9314p+0
	};
	for(int i=0;i<7;i++){
	  if(azz == wlist[i][0])
	    y1 = __builtin_copysign(wlist[i][1] + ((rm+sign == 2) ? 0x1p-52 : 0), zz);
	}
      }
    }
  }
  b64u64_u cvt3 = {.f = y1};
  cvt3.u += (uint64_t)(et - 342 - 1023)<<52;
  int64_t m0 = cvt3.u<<30, m1 = m0>>63;
  if(__builtin_expect((uint64_t)(m0^m1)<=(1ul<<30),0)){
    b64u64_u cvt4 = {.f = y1};
    cvt4.u = (cvt4.u + (1ul<<15))&0xffffffffffff0000ull;
    if( __builtin_fabs((cvt4.f - y1) - dy) < 0x1p-60 || __builtin_fabs(zz) == 1.0 ){
      cvt3.u = (cvt3.u + (1ul<<15))&0xffffffffffff0000ull;
    }
  }
  return cvt3.f;
}
libm_alias_double (__cbrt, cbrt)
