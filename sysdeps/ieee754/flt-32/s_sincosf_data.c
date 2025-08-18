/* Compute sine and cosine of argument.
   Copyright (C) 2018-2025 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <stdint.h>
#include <math.h>
#include "math_config.h"
#include "s_sincosf.h"

const double __sincosf_data_b[]
    = { 0x1.3bd3cc9be45dcp-6, -0x1.03c1f081b0833p-14, 0x1.55d3c6fc9ac1fp-24,
	-0x1.e1d3ff281b40dp-35 };
const double __sincosf_data_a[]
    = { 0x1.921fb54442d17p-3, -0x1.4abbce6256a39p-10, 0x1.466bc5a518c16p-19,
	-0x1.32bdc61074ff6p-29 };
const double __sincosf_data_tb[]
    ={  0x0p+0,                0x1.8f8b83c69a60bp-3,  0x1.87de2a6aea963p-2,
        0x1.1c73b39ae68c8p-1,  0x1.6a09e667f3bcdp-1,  0x1.a9b66290ea1a3p-1,
        0x1.d906bcf328d46p-1,  0x1.f6297cff75cbp-1,   0x1p+0,
        0x1.f6297cff75cbp-1,   0x1.d906bcf328d46p-1,  0x1.a9b66290ea1a3p-1,
        0x1.6a09e667f3bcdp-1,  0x1.1c73b39ae68c8p-1,  0x1.87de2a6aea963p-2,
        0x1.8f8b83c69a60bp-3,  0x0p+0,               -0x1.8f8b83c69a60bp-3,
       -0x1.87de2a6aea963p-2, -0x1.1c73b39ae68c8p-1, -0x1.6a09e667f3bcdp-1,
       -0x1.a9b66290ea1a3p-1, -0x1.d906bcf328d46p-1, -0x1.f6297cff75cbp-1,
       -0x1p+0,               -0x1.f6297cff75cbp-1,  -0x1.d906bcf328d46p-1,
       -0x1.a9b66290ea1a3p-1, -0x1.6a09e667f3bcdp-1, -0x1.1c73b39ae68c8p-1,
       -0x1.87de2a6aea963p-2, -0x1.8f8b83c69a60bp-3 };

const double __sincosf_data_tb_c[] = { 0x1p+0,
			     0x1.f6297cff75cbp-1,
			     0x1.d906bcf328d46p-1,
			     0x1.a9b66290ea1a3p-1,
			     0x1.6a09e667f3bcdp-1,
			     0x1.1c73b39ae68c8p-1,
			     0x1.87de2a6aea963p-2,
			     0x1.8f8b83c69a60bp-3,
			     0x0p+0,
			     -0x1.8f8b83c69a60bp-3,
			     -0x1.87de2a6aea963p-2,
			     -0x1.1c73b39ae68c8p-1,
			     -0x1.6a09e667f3bcdp-1,
			     -0x1.a9b66290ea1a3p-1,
			     -0x1.d906bcf328d46p-1,
			     -0x1.f6297cff75cbp-1,
			     -0x1p+0,
			     -0x1.f6297cff75cbp-1,
			     -0x1.d906bcf328d46p-1,
			     -0x1.a9b66290ea1a3p-1,
			     -0x1.6a09e667f3bcdp-1,
			     -0x1.1c73b39ae68c8p-1,
			     -0x1.87de2a6aea963p-2,
			     -0x1.8f8b83c69a60bp-3,
			     0x0p+0,
			     0x1.8f8b83c69a60bp-3,
			     0x1.87de2a6aea963p-2,
			     0x1.1c73b39ae68c8p-1,
			     0x1.6a09e667f3bcdp-1,
			     0x1.a9b66290ea1a3p-1,
			     0x1.d906bcf328d46p-1,
			     0x1.f6297cff75cbp-1 };

