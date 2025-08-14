/* Common definitions for expf/exp2f.

Copyright (c) 2023-2025 Alexei Sibidanov.

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

#include "e_expf_data.h"

const double __expf_data_tb[]
    = { 0x1.0000000000000p+0, 0x1.02c9a3e778061p+0, 0x1.059b0d3158574p+0,
	0x1.0874518759bc8p+0, 0x1.0b5586cf9890fp+0, 0x1.0e3ec32d3d1a2p+0,
	0x1.11301d0125b51p+0, 0x1.1429aaea92de0p+0, 0x1.172b83c7d517bp+0,
	0x1.1a35beb6fcb75p+0, 0x1.1d4873168b9aap+0, 0x1.2063b88628cd6p+0,
	0x1.2387a6e756238p+0, 0x1.26b4565e27cddp+0, 0x1.29e9df51fdee1p+0,
	0x1.2d285a6e4030bp+0, 0x1.306fe0a31b715p+0, 0x1.33c08b26416ffp+0,
	0x1.371a7373aa9cbp+0, 0x1.3a7db34e59ff7p+0, 0x1.3dea64c123422p+0,
	0x1.4160a21f72e2ap+0, 0x1.44e086061892dp+0, 0x1.486a2b5c13cd0p+0,
	0x1.4bfdad5362a27p+0, 0x1.4f9b2769d2ca7p+0, 0x1.5342b569d4f82p+0,
	0x1.56f4736b527dap+0, 0x1.5ab07dd485429p+0, 0x1.5e76f15ad2148p+0,
	0x1.6247eb03a5585p+0, 0x1.6623882552225p+0, 0x1.6a09e667f3bcdp+0,
	0x1.6dfb23c651a2fp+0, 0x1.71f75e8ec5f74p+0, 0x1.75feb564267c9p+0,
	0x1.7a11473eb0187p+0, 0x1.7e2f336cf4e62p+0, 0x1.82589994cce13p+0,
	0x1.868d99b4492edp+0, 0x1.8ace5422aa0dbp+0, 0x1.8f1ae99157736p+0,
	0x1.93737b0cdc5e5p+0, 0x1.97d829fde4e50p+0, 0x1.9c49182a3f090p+0,
	0x1.a0c667b5de565p+0, 0x1.a5503b23e255dp+0, 0x1.a9e6b5579fdbfp+0,
	0x1.ae89f995ad3adp+0, 0x1.b33a2b84f15fbp+0, 0x1.b7f76f2fb5e47p+0,
	0x1.bcc1e904bc1d2p+0, 0x1.c199bdd85529cp+0, 0x1.c67f12e57d14bp+0,
	0x1.cb720dcef9069p+0, 0x1.d072d4a07897cp+0, 0x1.d5818dcfba487p+0,
	0x1.da9e603db3285p+0, 0x1.dfc97337b9b5fp+0, 0x1.e502ee78b3ff6p+0,
	0x1.ea4afa2a490dap+0, 0x1.efa1bee615a27p+0, 0x1.f50765b6e4540p+0,
	0x1.fa7c1819e90d8p+0 };

const double __expf_c[]
    = { 0x1.62e42fefa39efp-1, 0x1.ebfbdff82c58fp-3,  0x1.c6b08d702e0edp-5,
	0x1.3b2ab6fb92e5ep-7, 0x1.5d886e6d54203p-10, 0x1.430976b8ce6efp-13 };
const double __expf_b[]
    = { 1, 0x1.62e42fef4c4e7p-1, 0x1.ebfd1b232f475p-3, 0x1.c6b19384ecd93p-5 };

const double __exp2f_b[]
    = { 1, 0x1.62e42fef4c4e7p-1, 0x1.ebfd1b232f475p-3, 0x1.c6b19384ecd93p-5 };

const double __exp2f_c[]
    = { 0x1.62e42fefa39efp-1, 0x1.ebfbdff82c58fp-3,  0x1.c6b08d702e0edp-5,
	0x1.3b2ab6fb92e5ep-7, 0x1.5d886e6d54203p-10, 0x1.430976b8ce6efp-13 };
