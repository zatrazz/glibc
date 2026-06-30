/* Data tables for the e_atan2 implementation.

Copyright (c) 2024-2025 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/atan2/atan2.c, revision 8ea8ea35).

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

#ifndef _E_ATAN2_DATA_H
#define _E_ATAN2_DATA_H

#include "atan2_tint.h"

extern const tint_t __atan2_data_P[30] attribute_hidden;
#define P __atan2_data_P

extern const tint_t __atan2_data_Q[30] attribute_hidden;
#define Q __atan2_data_Q

extern const double __atan2_data_T2[] attribute_hidden;
#define T2 __atan2_data_T2

extern const double __atan2_data_f2[][2] attribute_hidden;
#define f2 __atan2_data_f2

extern const double __atan2_data_O[8][2] attribute_hidden;
#define O __atan2_data_O

#endif
