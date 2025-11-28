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

#ifndef _E_COSHSINH_COMMON_H
#define _E_COSHSINH_COMMON_H

#include <ddcoremath.h>

static inline double
polydd_coshsinh (double xh, double xl, int n, const double c[][2], double *l)
{
  int i = n - 1;
  double ch = c[i][0] + *l, cl = ((c[i][0] - ch) + *l) + c[i][1], e;
  while (--i >= 0)
    {
      ch = muldd2 (xh, xl, ch, cl, &cl);
      ch = fasttwosum (c[i][0], ch, &e);
      cl = (cl + c[i][1]) + e;
    }
  *l = cl;
  return ch;
}

static double __attribute__ ((noinline))
as_exp_accurate (double x, double t, double th, double tl, double *l)
{
  static const double ch[][2]
      = { { 0x1p+0, 0x1.6c16bd194535dp-94 },
	  { 0x1p-1, -0x1.8259d904fd34fp-93 },
	  { 0x1.5555555555555p-3, 0x1.53e93e9f26e62p-57 } };
  const double l2h = 0x1.62e42ffp-13, l2l = 0x1.718432a1b0e26p-47,
	       l2ll = 0x1.9ff0342542fc3p-102;
  double dx = x - l2h * t, dxl = l2l * t, dxll = l2ll * t + fma (l2l, t, -dxl);
  double dxh = dx + dxl;
  dxl = ((dx - dxh) + dxl) + dxll;
  double fl = dxh
	      * (0x1.5555555555555p-5
		 + dxh * (0x1.11111113e93e9p-7 + dxh * 0x1.6c16c169400a7p-10));
  double fh = polydd_coshsinh (dxh, dxl, 3, ch, &fl);
  fh = muldd2 (dxh, dxl, fh, fl, &fl);
  fh = muldd2 (th, tl, fh, fl, &fl);
  double zh = th + fh, zl = (th - zh) + fh;
  double uh = zh + tl, ul = ((zh - uh) + tl) + zl;
  double vh = uh + fl, vl = ((uh - vh) + fl) + ul;
  *l = vl;
  return vh;
}

#endif
