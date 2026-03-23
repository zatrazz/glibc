/* Correctly-rounded sine of binary32 value.
 
Copyright (c) 2022-2026 Alexei Sibidanov.
 
The original version of this file was copied from the CORE-MATH
project (file src/binary32/cosh/coshf.c, revision 8ea8ea35.

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

#include <s_sincosf_data.h>

const uint64_t __sinf_ipi[] =
{
  0xfe5163abdebbc562, 0xdb6295993c439041, 0xfc2757d1f534ddc0,
  0xa2f9836e4e441529
};

const double __sinf_b[] =
{
   0x1.3bd3cc9be45dcp-6, -0x1.03c1f081b0833p-14,  0x1.55d3c6fc9ac1fp-24,
  -0x1.e1d3ff281b40dp-35
};
const double __sinf_a[] =
{
   0x1.921fb54442d17p-3, -0x1.4abbce6256a39p-10,   0x1.466bc5a518c16p-19,
  -0x1.32bdc61074ff6p-29
};
const double __sinf_tb[] =
{
   0x0p+0,                0x1.8f8b83c69a60bp-3,  0x1.87de2a6aea963p-2,
   0x1.1c73b39ae68c8p-1,  0x1.6a09e667f3bcdp-1,  0x1.a9b66290ea1a3p-1,
   0x1.d906bcf328d46p-1,  0x1.f6297cff75cbp-1,   0x1p+0,
   0x1.f6297cff75cbp-1,   0x1.d906bcf328d46p-1,  0x1.a9b66290ea1a3p-1,
   0x1.6a09e667f3bcdp-1,  0x1.1c73b39ae68c8p-1,  0x1.87de2a6aea963p-2,
   0x1.8f8b83c69a60bp-3,  0x0p+0,               -0x1.8f8b83c69a60bp-3,
  -0x1.87de2a6aea963p-2, -0x1.1c73b39ae68c8p-1, -0x1.6a09e667f3bcdp-1,
  -0x1.a9b66290ea1a3p-1, -0x1.d906bcf328d46p-1, -0x1.f6297cff75cbp-1,
  -0x1p+0,               -0x1.f6297cff75cbp-1,  -0x1.d906bcf328d46p-1,
  -0x1.a9b66290ea1a3p-1, -0x1.6a09e667f3bcdp-1, -0x1.1c73b39ae68c8p-1,
  -0x1.87de2a6aea963p-2, -0x1.8f8b83c69a60bp-3
};

const double __cosf_tb[] =
{
   0x1p+0,                0x1.f6297cff75cbp-1,   0x1.d906bcf328d46p-1,
   0x1.a9b66290ea1a3p-1,  0x1.6a09e667f3bcdp-1,  0x1.1c73b39ae68c8p-1,
   0x1.87de2a6aea963p-2,  0x1.8f8b83c69a60bp-3,  0x0p+0,
  -0x1.8f8b83c69a60bp-3, -0x1.87de2a6aea963p-2, -0x1.1c73b39ae68c8p-1,
  -0x1.6a09e667f3bcdp-1, -0x1.a9b66290ea1a3p-1, -0x1.d906bcf328d46p-1,
  -0x1.f6297cff75cbp-1,  -0x1p+0,               -0x1.f6297cff75cbp-1,
  -0x1.d906bcf328d46p-1, -0x1.a9b66290ea1a3p-1, -0x1.6a09e667f3bcdp-1,
  -0x1.1c73b39ae68c8p-1, -0x1.87de2a6aea963p-2, -0x1.8f8b83c69a60bp-3,
   0x0p+0,                0x1.8f8b83c69a60bp-3,  0x1.87de2a6aea963p-2,
   0x1.1c73b39ae68c8p-1,  0x1.6a09e667f3bcdp-1,  0x1.a9b66290ea1a3p-1,
   0x1.d906bcf328d46p-1,  0x1.f6297cff75cbp-1
};

const sincosf_database_t __sinf_st[] =
{
  { { 0x1.33333p+13 }, -0x1.63f4bap-2, -0x1p-27 },
  { { 0x1.75b8a2p-1 }, 0x1.55688ap-1, -0x1p-26 },
  { { 0x1.4f0654p+0 }, 0x1.ee836cp-1, -0x1p-26 },
  { { 0x1.2d97c8p+3 }, -0x1.99bc5ap-26, -0x1p-51 },
};

const sincosf_database_t __cosf_st[] =
{
  { { 0x1.2d97c8p+2 }, 0x1.99bc5cp-27, -0x1p-52 },
  { { 0x1.4555p+51 }, 0x1.115d7ep-1, -0x1p-26 },
  { { 0x1.48a858p+54 }, 0x1.f48148p-2, 0x1p-27 },
  { { 0x1.3170fp+63 }, 0x1.fe2976p-1, 0x1p-26 },
  { { 0x1.2b9622p+67 }, 0x1.f0285ep-1, -0x1p-26 },
};

const sincosf2_database_t __sincosf_st[] =
{
  { { 0x1.33333p+13 }, -0x1.63f4bap-2, -0x1p-27, -0x1.e01216p-1, -0x1p-26 },
  { { 0x1.75b8a2p-1 }, 0x1.55688ap-1, -0x1p-26, 0x1.7d8e1ep-1, 0x1p-26 },
  { { 0x1.4f0654p+0 }, 0x1.ee836cp-1, -0x1p-26, 0x1.09558p-2, -0x1p-27 },
  { { 0x1.2d97c8p+3 }, -0x1.99bc5ap-26, -0x1p-51, -0x1p+0, 0x1p-25 },
  { { 0x1.2d97c8p+2 }, -0x1p+0, 0x1p-25, 0x1.99bc5cp-27, -0x1p-52 },
  { { 0x1.4555p+51 }, -0x1.b0ea44p-1, 0x1p-26, 0x1.115d7ep-1, -0x1p-26 },
  { { 0x1.48a858p+54 }, 0x1.beac8cp-1, 0x1p-26, 0x1.f48148p-2, 0x1p-27 },
  { { 0x1.3170fp+63 }, 0x1.5ac1eep-4, -0x1p-30, 0x1.fe2976p-1, 0x1p-26 },
  { { 0x1.2b9622p+67 }, -0x1.f983c2p-3, 0x1p-28, 0x1.f0285ep-1, -0x1p-26 },
};
