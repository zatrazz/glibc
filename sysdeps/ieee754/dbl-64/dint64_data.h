/* Shared lookup tables for the CORE-MATH binary64 sin, cos and tan
   implementations.

Copyright (c) 2022-2025 Paul Zimmermann and Tom Hubrecht

These tables were copied from the CORE-MATH project (file
   src/binary64/cos/cos.c, revision 8ea8ea35).

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

#ifndef _DINT64_DATA_H
#define _DINT64_DATA_H

#include "dint64.h"

extern const uint64_t __dint64_data_T[20] attribute_hidden;
#define T __dint64_data_T

extern const dint64_t __dint64_data_S[256] attribute_hidden;
#define S __dint64_data_S

extern const dint64_t __dint64_data_C[256] attribute_hidden;
#define C __dint64_data_C

extern const double __dint64_data_PSfast[] attribute_hidden;
#define PSfast __dint64_data_PSfast

extern const double __dint64_data_PCfast[] attribute_hidden;
#define PCfast __dint64_data_PCfast

extern const dint64_t __dint64_data_PS[] attribute_hidden;
#define PS __dint64_data_PS

extern const dint64_t __dint64_data_PC[] attribute_hidden;
#define PC __dint64_data_PC

extern const double __dint64_data_SC[256][3] attribute_hidden;
#define SC __dint64_data_SC

#endif /* _DINT64_DATA_H */
