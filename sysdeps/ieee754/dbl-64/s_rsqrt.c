/* Correctly-rounded reciprocal square root of binary64 value.

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

#include <math.h>
#include <errno.h>
#include <fenv.h>
#include <stdint.h>
#include <get-rounding-mode.h>
#include <libm-alias-double.h>
#include <math_uint128.h>
#define CORE_MATH_SUPPORT_ERRNO

typedef uint64_t u64;
typedef int64_t i64;
typedef union {double f; uint64_t u;} b64u64_u;

static double __attribute__((noinline)) as_rsqrt_refine(double rf, double a){
  b64u64_u ir = {.f = rf}, ia = {.f = a};
  if(ia.u < 1ll<<52){
    i64 nz = __builtin_clzll(ia.u);
    ia.u <<= nz - 11;
    ia.u &= ~0ull>>12;
    i64 e = nz - 12;
    ia.u |= e<<52;
  }
  if(ia.u<<11 == 1ull<<63){
  } else {
    unsigned mode = get_rounding_mode ();
    int e = (ia.u>>52)&1;
    u64 rm, am;
    rm = (ir.u<<11|1ull<<63)>>11;
    am = ((ia.u&(~0ull>>12))|1ull<<52)<<(5-e);
    u128 rt = u128_mul (u128_from_u64 (rm), u128_from_u64 (am));
    u64 rth = u128_high (rt), rtl = u128_low (rt);
    u128 rrt = u128_mul (u128_from_u64 (rtl), u128_from_u64 (rm));
    u64 t0 = u128_low (rrt), t1 = u128_high (rrt) + rth*rm;
    rrt = u128_from_u64_hl (t1, t0);
    i64 s = u128_low (u128_rshift (rrt, 127)), dd = 1 - 2*s;
    u128 rts = u128_add (u128_bitwise_xor (u128_lshift (rt, 1),
					   u128_from_i64 (-s)),
			 u128_from_i64 (s));
    u128 prrt;
    u64 am2 = am<<1, am20 = -am;
    do {
      ir.u -= dd;
      prrt = rrt;
      am20 += am2;
      u128 tt = u128_sub (rts, u128_from_u64 (am20));
      //rrt -= tt;
      rrt = u128_sub (rrt, tt);
    } while (u128_logical_not (u128_rshift(u128_bitwise_xor (prrt,
							     rrt),127)));
    ir.u += u128_low (u128_rshift (rrt, 127))?0:dd;
    rrt = u128_low (u128_rshift (rrt, 127)) ? rrt : prrt;

    if(__builtin_expect(mode==FE_TONEAREST, 1)){
      rm = (ir.u<<11|1ull<<63)>>11;
      rt = u128_mul(u128_from_u64 (rm), u128_from_u64 (am));
      rrt = u128_add (rrt, u128_from_u64 (am >> 2));
      rrt = u128_add (rrt, rt);
      u64 inc = u128_low (u128_rshift (rrt, 127));
      ir.u += inc;
    } else {
      ir.u += mode==FE_UPWARD;
    }
    rf = ir.f;
  }
  return rf;
}

double __rsqrt(double x)
{
  b64u64_u ix = {.f = x};
  double r;
  if(__builtin_expect(ix.u < 1ll<<52, 0)){ // 0 <= x < 0x1p-1022
    if(__builtin_expect(ix.u, 1)){ // x <> +0
      r = __builtin_sqrt(x)/x;
    } else {
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = ERANGE; // pole error
#endif
      return 1.0 / 0.0; // case x = +0
    }
  } else if(__builtin_expect(ix.u >= 0x7ffull<<52, 0)){ // NaN, Inf, x <= 0
    if(!(ix.u<<1)) {
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = ERANGE; // pole error
#endif
      return 1.0 / -0.0; // x=-0
    }
    if(ix.u > 0xfff0000000000000ull) return x + x; // -NaN
    if(ix.u >> 63){ // x < 0
#ifdef CORE_MATH_SUPPORT_ERRNO
      errno = EDOM;
#endif
      feraiseexcept (FE_INVALID);
      return __builtin_nan("");
    }
    if(!(ix.u<<12)) return 0.0; // +/-Inf
    return x + x; // +NaN
  } else { // 0x1p-1022 <= x < 2^1024
    if (__builtin_expect (ix.u > 0x7fd000000000000ull, 0)) // x > 2^1022
      // avoid spurious underflow in 1/x
      r = (4.0 / x) * (0.25 * __builtin_sqrt(x));
    else
      r = (1.0 / x) * __builtin_sqrt(x);
  }
  double rx = r*x, drx = __builtin_fma(r, x, -rx);
  double h = __builtin_fma(r,rx,-1.0) + r*drx, dr = (r*0.5)*h;
  double rf = r - dr;
  dr -= r - rf;
  b64u64_u idr = {.f = dr}, ir = {.f = rf};
  u64 aidr = (idr.u&(~0ull>>1)) - (ir.u & (0x7ffll<<52)) + (0x3fell<<52), mid = (aidr - 0x3c90000000000000 + 16)>>5;
  if(__builtin_expect( mid==0 || aidr<0x39b0000000000000ll || aidr>0x3c9fffffffffff80ll, 0))
    rf = as_rsqrt_refine(rf, x);
  return rf;
}
libm_alias_double (__rsqrt, rsqrt)
