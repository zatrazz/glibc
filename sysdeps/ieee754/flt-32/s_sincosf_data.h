/* Compute sine and cosine of argument.
   Copyright (C) 2018-2026 Free Software Foundation, Inc.
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

#ifndef _S_SINCOSF_DATA_H
#define _S_SINCOSF_DATA_H

#include <stdint.h>

extern const uint64_t __sinf_ipi[] attribute_hidden;
#define IPI __sinf_ipi

/* Two doubles, the sine and the cosine in sincosf; the arithmetic on it
   is the same in every lane, so the compiler either vectorizes it or
   emits the two scalar computations.  */
typedef double sincosf_v2df_t __attribute__ ((__vector_size__ (16)));

typedef struct
{
  double b[4];
  double a[4];
  double tb[32];
  double tb_cosf[32];
  /* For sincosf: { a[k], b[k] }, and rows of the pairs { c, -s } and
     { -s, -c } with s = tb[k] and c = tb[k + 8] the sine and cosine of
     k*pi/16 (the table is antisymmetric, -s = tb[k + 16] and
     -c = tb[k + 24]).  */
  sincosf_v2df_t ab[4];
  sincosf_v2df_t tbr[32][2];
} sincosf_tables_t;
extern const sincosf_tables_t __sincosf_tables attribute_hidden;
#define B __sincosf_tables.b
#define A __sincosf_tables.a
#define TB __sincosf_tables.tb
#define TB_COSF __sincosf_tables.tb_cosf
#define AB __sincosf_tables.ab
#define TBR __sincosf_tables.tbr

typedef struct
{
  union
  {
    float arg;
    uint32_t uarg;
  };
  float rh, rl;
} sincosf_database_t;
extern const sincosf_database_t __sinf_st[4] attribute_hidden;
#define ST __sinf_st
extern const sincosf_database_t __cosf_st[5] attribute_hidden;
#define ST_COSF __cosf_st

typedef struct
{
  union
  {
    float arg;
    uint32_t uarg;
  };
  float sh, sl;
  float ch, cl;
} sincosf2_database_t;
extern const sincosf2_database_t __sincosf_st[9] attribute_hidden;
#define ST_SINCOSF __sincosf_st

#endif
