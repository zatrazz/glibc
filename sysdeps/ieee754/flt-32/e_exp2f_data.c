/* Shared data between expf, exp2f and powf.
   Copyright (C) 2017-2025 Free Software Foundation, Inc.
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

#include "math_config.h"

const uint64_t __expf2_tb[] = {
  0x3ff0000000000000, 0x3ff02c9a3e778061, 0x3ff059b0d3158574,
  0x3ff0874518759bc8, 0x3ff0b5586cf9890f, 0x3ff0e3ec32d3d1a2,
  0x3ff11301d0125b51, 0x3ff1429aaea92de0, 0x3ff172b83c7d517b,
  0x3ff1a35beb6fcb75, 0x3ff1d4873168b9aa, 0x3ff2063b88628cd6,
  0x3ff2387a6e756238, 0x3ff26b4565e27cdd, 0x3ff29e9df51fdee1,
  0x3ff2d285a6e4030b, 0x3ff306fe0a31b715, 0x3ff33c08b26416ff,
  0x3ff371a7373aa9cb, 0x3ff3a7db34e59ff7, 0x3ff3dea64c123422,
  0x3ff4160a21f72e2a, 0x3ff44e086061892d, 0x3ff486a2b5c13cd0,
  0x3ff4bfdad5362a27, 0x3ff4f9b2769d2ca7, 0x3ff5342b569d4f82,
  0x3ff56f4736b527da, 0x3ff5ab07dd485429, 0x3ff5e76f15ad2148,
  0x3ff6247eb03a5585, 0x3ff6623882552225, 0x3ff6a09e667f3bcd,
  0x3ff6dfb23c651a2f, 0x3ff71f75e8ec5f74, 0x3ff75feb564267c9,
  0x3ff7a11473eb0187, 0x3ff7e2f336cf4e62, 0x3ff82589994cce13,
  0x3ff868d99b4492ed, 0x3ff8ace5422aa0db, 0x3ff8f1ae99157736,
  0x3ff93737b0cdc5e5, 0x3ff97d829fde4e50, 0x3ff9c49182a3f090,
  0x3ffa0c667b5de565, 0x3ffa5503b23e255d, 0x3ffa9e6b5579fdbf,
  0x3ffae89f995ad3ad, 0x3ffb33a2b84f15fb, 0x3ffb7f76f2fb5e47,
  0x3ffbcc1e904bc1d2, 0x3ffc199bdd85529c, 0x3ffc67f12e57d14b,
  0x3ffcb720dcef9069, 0x3ffd072d4a07897c, 0x3ffd5818dcfba487,
  0x3ffda9e603db3285, 0x3ffdfc97337b9b5f, 0x3ffe502ee78b3ff6,
  0x3ffea4afa2a490da, 0x3ffefa1bee615a27, 0x3fff50765b6e4540,
  0x3fffa7c1819e90d8
};

const double __expf2_b[] = {
  1, 0x1.62e42fef4c4e7p-1, 0x1.ebfd1b232f475p-3, 0x1.c6b19384ecd93p-5
};

const double __expf2_c[] = {
  0x1.62e42fefa39efp-1, 0x1.ebfbdff82c58fp-3,  0x1.c6b08d702e0edp-5,
  0x1.3b2ab6fb92e5ep-7, 0x1.5d886e6d54203p-10, 0x1.430976b8ce6efp-13
};
