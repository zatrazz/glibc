/* Internal data for atan2 function for two binary64 values.

Copyright (c) 2024-2025 Paul Zimmermann and Alexei Sibidanov

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

#ifndef _S_ATAN2_DATA_H
#define _S_ATAN2_DATA_H

#include "tint.h"

extern const tint_t __atan2_P[30] attribute_hidden;
extern const tint_t __atan2_Q[30] attribute_hidden;
extern const double __atan2_T2[] attribute_hidden;
extern const double __atan2_F2[][2] attribute_hidden;
extern const double __atan2_O[8][2] attribute_hidden;

#define P  __atan2_P
#define Q  __atan2_P
#define T2 __atan2_T2
#define F2 __atan2_F2
#define O  __atan2_O

#endif
