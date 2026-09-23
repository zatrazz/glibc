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

typedef double sincosf_v2df_t __attribute__ ((__vector_size__ (16)));

typedef struct
{
  /* { cos(k*pi/16), -sin(k*pi/16) }, with the first 8 entries repeated
     at the end so CS[(i & 31) + 8] needs no wrap around.  */
  sincosf_v2df_t cs[40];
  /* The coefficients { a[k], b[k] } of the two polynomials in z^2.  */
  sincosf_v2df_t ab[4];
} sincosf_tables_t;
extern const sincosf_tables_t __sincosf_tables attribute_hidden;
#define AB __sincosf_tables.ab
#define A(k) AB[k][0]
#define B(k) AB[k][1]
#define CS __sincosf_tables.cs

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
