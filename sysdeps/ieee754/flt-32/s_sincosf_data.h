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

/* The polynomial coefficients and the sin/cos tables in one object, so
   a function computes a single base address for all of them.  */
typedef struct
{
  double b[4];
  double a[4];
  double tb[32];
  double tb_cosf[32];
} sincosf_tables_t;
extern const sincosf_tables_t __sincosf_tables attribute_hidden;
#define B __sincosf_tables.b
#define A __sincosf_tables.a
#define TB __sincosf_tables.tb
#define TB_COSF __sincosf_tables.tb_cosf

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
