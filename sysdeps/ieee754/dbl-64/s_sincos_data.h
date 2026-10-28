/* Tables shared by the correctly-rounded binary64 sin and cos.

Copyright (c) 2026 Paul Zimmermann and Alexei Sibidanov.

The original version of these tables was copied from the CORE-MATH
project (files src/binary64/sin/sin.c, revision a3fe413f, and
src/binary64/cos/cos.c, revision 9b821fc6).

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

#ifndef _S_SINCOS_DATA_H
#define _S_SINCOS_DATA_H

#include <stdint.h>

/* 1/(2pi) with precision 1280, in 64-bit words (the first one is 0).  */
extern const uint64_t __sincos_inv2pi[20] attribute_hidden;
#define T   __sincos_inv2pi

/* The sin and cos tables: the 128-bit numbers are stored as { high, low }
   pairs.  */
extern const uint64_t __sin_ps[4][2] attribute_hidden;
#define PS  __sin_ps
extern const uint64_t __sin_pc[5][2] attribute_hidden;
#define PC  __sin_pc
extern const double __sin_u1[128][4] attribute_hidden;
#define U1  __sin_u1
extern const double __sin_u2[128][4] attribute_hidden;
#define U2  __sin_u2
extern const uint64_t __sin_s1u[33][2] attribute_hidden;
#define S1u __sin_s1u
extern const uint64_t __sin_s2u[64][2] attribute_hidden;
#define S2u __sin_s2u
extern const uint64_t __sin_c2u[64][2] attribute_hidden;
#define C2u __sin_c2u

#endif
