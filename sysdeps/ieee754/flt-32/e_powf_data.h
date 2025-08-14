/* Internal data for powf implementation.

Copyright (c) 2022-2025 Alexei Sibidanov and Paul Zimmermann

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

#ifndef _E_POWF_DATA_H
#define _E_POWF_DATA_H

#include <stdint.h>

extern const uint32_t __powf_data_xmax[] attribute_hidden;
#define XMAX __powf_data_xmax
extern const double __powf_data_ix[] attribute_hidden;
#define IX __powf_data_ix
extern const double __powf_data_lix[][2] attribute_hidden;
#define LIX __powf_data_lix
extern const double __powf_data_c[] attribute_hidden;
#define C __powf_data_c
extern const double __powf_data_ce[] attribute_hidden;
#define CE __powf_data_ce
extern const double __powf_data_tb[] attribute_hidden;
#define TB __powf_data_tb
extern const double __powf_data_ch[][2] attribute_hidden;
#define CH __powf_data_ch
extern const double __powf_data_ce2[][2] attribute_hidden;
#define CE2 __powf_data_ce2

#endif
