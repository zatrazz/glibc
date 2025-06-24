/* Correctly-rounded sine/cosine function for binary64 value.

Copyright (c) 2022-2025 Paul Zimmermann and Tom Hubrecht

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

#ifndef _SINCOS_DATA_H
#define _SINCOS_DATA_H

#include <stdint.h>
#include <math_uint128.h>
#include "dint.h"

extern const uint64_t __sincos_T[20] attribute_hidden;
extern const dint64_t __sincos_S[256] attribute_hidden;

extern const dint64_t __sincos_C[256] attribute_hidden;
extern const double __sincos_PSfast[] attribute_hidden;
extern const double __sincos_PCfast[] attribute_hidden;

extern const dint64_t __sincos_PS[] attribute_hidden;
extern const dint64_t __sincos_PC[] attribute_hidden;
extern const double __sincos_SC[256][3] attribute_hidden;

#define T       __sincos_T
#define S       __sincos_S

#define C       __sincos_C
#define PSfast  __sincos_PSfast
#define PCfast  __sincos_PCfast

#define PS      __sincos_PS
#define PC      __sincos_PC
#define SC      __sincos_SC

#endif
