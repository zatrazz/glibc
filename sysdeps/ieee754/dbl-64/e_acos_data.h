/* Data tables for the acos implementation.

Copyright (c) 2024-2025 Alexei Sibidanov.

The original version of this file was copied from the CORE-MATH
project (file src/binary64/acos/acos.c, revision b748a2d2).

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

#ifndef _E_ACOS_DATA_H
#define _E_ACOS_DATA_H

extern const double __acos_data_cc[33][8] attribute_hidden;
#define cc __acos_data_cc

extern const double __acos_data_off[][2] attribute_hidden;
#define off __acos_data_off

extern const double __acos_data_s[33][2] attribute_hidden;
#define SINCOS __acos_data_s

extern const double __acos_data_c[][2] attribute_hidden;
#define POLYC __acos_data_c

extern const double __acos_data_ct[] attribute_hidden;
#define ct __acos_data_ct

#endif
