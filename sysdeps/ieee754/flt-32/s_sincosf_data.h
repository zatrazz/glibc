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

extern const uint64_t __sinf_ipi[] attribute_hidden;
#define IPI __sinf_ipi

extern const double __sinf_b[] attribute_hidden;
#define B __sinf_b
extern const double __sinf_a[] attribute_hidden;
#define A __sinf_a
extern const double __sinf_tb[] attribute_hidden;
#define TB __sinf_tb

extern const double __cosf_tb[] attribute_hidden;
#define TB_COSF __cosf_tb

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
