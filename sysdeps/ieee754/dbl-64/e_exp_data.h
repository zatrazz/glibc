/* Correctly rounded exponential function for binary64 values.

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

#ifndef _E_EXP_DATA_H
#define _E_EXP_DATA_H

extern const double __exp_data_db[] attribute_hidden;
extern const size_t __exp_data_db_size attribute_hidden;
#define DB       __exp_data_db
#define DB_SIZE  __exp_data_db_size

extern const double __exp_data_t0[][2] attribute_hidden;
extern const double __exp_data_t1[][2] attribute_hidden;
#define T0       __exp_data_t0
#define T1       __exp_data_t1

#endif
