/* Common data for atan.

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

#ifndef _S_ATAN_DATA_H
#define _S_ATAN_DATA_H

#include <stddef.h>
#include <stdint.h>

extern const double __atan_A[][2] attribute_hidden;
extern const uint16_t __atan_c[31][3] attribute_hidden;
extern const double __atan_db[][3] attribute_hidden;
extern const size_t __atan_db_size attribute_hidden;
extern const double __atan_ch[][2] attribute_hidden;
extern const double __atan_cl[] attribute_hidden;

#define A       __atan_A
#define C       __atan_c
#define DB      __atan_db
#define DB_SIZE __atan_db_size
#define CH      __atan_ch
#define CL      __atan_cl

#endif
