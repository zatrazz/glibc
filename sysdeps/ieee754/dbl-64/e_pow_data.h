/* Common data for binary64 pow implementation.

Copyright (c) 2022-2025 CERN and Inria
Authors: Tom Hubrecht and Paul Zimmermann

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

#ifndef _E_POW_DATA_H
#define _E_POW_DATA_H

#include <math_uint128.h>
#include <endian.h>

/*
  Type and structure definitions
*/

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
typedef union {
  struct {
    u128 r;
    int64_t _ex;
    uint64_t _sgn;
  };
  struct {
    uint64_t lo;
    uint64_t hi;
    int64_t ex;
    uint64_t sgn;
  };
} dint64_t;
#else
typedef union {
  struct {
    u128 r;
    int64_t _ex;
    uint64_t _sgn;
  };
  struct {
    uint64_t hi;
    uint64_t lo;
    int64_t ex;
    uint64_t sgn;
  };
} dint64_t;
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
typedef union {
  /* Use a little-endian representation.
     FIXME: adapt for big-endian processors. */
  struct {
    u128 rl;
    u128 rh;
    int64_t _ex;
    uint64_t _sgn;
  };
  struct {
    uint64_t ll; /* lower low part */
    uint64_t lh; /* upper low part */
    uint64_t hl; /* lower high part */
    uint64_t hh; /* upper high part */
    int64_t ex;
    uint64_t sgn;
  };
} qint64_t;
#else
typedef union {
  struct {
    u128 rl;
    u128 rh;
    int64_t _ex;
    uint64_t _sgn;
  };
  struct {
    uint64_t lh; /* upper low part */
    uint64_t ll; /* lower low part */
    uint64_t hh; /* upper high part */
    uint64_t hl; /* lower high part */
    int64_t ex;
    uint64_t sgn;
  };
} qint64_t;
#endif


extern const double __pow_data_INVERSE[182] attribute_hidden;
#define _INVERSE __pow_data_INVERSE

extern const double __pow_data_LOG_INV[182][2] attribute_hidden;
#define _LOG_INV __pow_data_LOG_INV

extern const double __pow_data_T1[][2] attribute_hidden;
#define T1 __pow_data_T1

extern const double __pow_data_T2[][2] attribute_hidden;
#define T2 __pow_data_T2

extern const double __pow_data_P_1[] attribute_hidden;
#define P_1 __pow_data_P_1

extern const double __pow_data_Q_1[] attribute_hidden;
#define Q_1 __pow_data_Q_1


extern const dint64_t __pow_data_ONE attribute_hidden;
#define ONE __pow_data_ONE

extern const dint64_t __pow_data_M_ONE attribute_hidden;
#define M_ONE __pow_data_M_ONE

extern const dint64_t __pow_data_LOG2 attribute_hidden;
#define LOG2 __pow_data_LOG2

extern const dint64_t __pow_data_LOG2_INV attribute_hidden;
#define LOG2_INV __pow_data_LOG2_INV

extern const dint64_t __pow_data_ZERO attribute_hidden;
#define ZERO __pow_data_ZERO

extern const dint64_t __pow_data_INVERSE_2_1[] attribute_hidden;
#define _INVERSE_2_1 __pow_data_INVERSE_2_1

extern const dint64_t __pow_data_INVERSE_2_2[] attribute_hidden;
#define _INVERSE_2_2 __pow_data_INVERSE_2_2

extern const dint64_t __pow_data_LOG_INV_2_1[] attribute_hidden;
#define _LOG_INV_2_1 __pow_data_LOG_INV_2_1

extern const dint64_t __pow_data_LOG_INV_2_2[] attribute_hidden;
#define _LOG_INV_2_2 __pow_data_LOG_INV_2_2

extern const dint64_t __pow_data_T1_2[] attribute_hidden;
#define T1_2 __pow_data_T1_2

extern const dint64_t __pow_data_T2_2[] attribute_hidden;
#define T2_2 __pow_data_T2_2

extern const dint64_t __pow_data_P_2[] attribute_hidden;
#define P_2 __pow_data_P_2

extern const dint64_t __pow_data_Q_2[] attribute_hidden;
#define Q_2 __pow_data_Q_2


extern const qint64_t __pow_data_ONE_Q attribute_hidden;
#define ONE_Q __pow_data_ONE_Q
extern const qint64_t __pow_data_M_ONE_Q attribute_hidden;
#define M_ONE_Q __pow_data_M_ONE_Q
extern const qint64_t __pow_data_LOG2_Q attribute_hidden;
#define LOG2_Q __pow_data_LOG2_Q
extern const qint64_t __pow_data_LOG2_INV_Q attribute_hidden;
#define LOG2_INV_Q __pow_data_LOG2_INV_Q
extern const qint64_t __pow_data_ZERO_Q attribute_hidden;
#define ZERO_Q __pow_data_ZERO_Q

extern const qint64_t __pow_data_INVERSE_3_1[] attribute_hidden;
#define _INVERSE_3_1 __pow_data_INVERSE_3_1
extern const qint64_t __pow_data_INVERSE_3_2[] attribute_hidden;
#define _INVERSE_3_2 __pow_data_INVERSE_3_2
extern const qint64_t __pow_data_LOG_INV_3_1[] attribute_hidden;
#define _LOG_INV_3_1 __pow_data_LOG_INV_3_1
extern const qint64_t __pow_data_LOG_INV_3_2[] attribute_hidden;
#define _LOG_INV_3_2 __pow_data_LOG_INV_3_2
extern const qint64_t __pow_data_T1_3[] attribute_hidden;
#define T1_3 __pow_data_T1_3
extern const qint64_t __pow_data_T2_3[] attribute_hidden;
#define T2_3 __pow_data_T2_3
extern const qint64_t __pow_data_P_3[] attribute_hidden;
#define P_3 __pow_data_P_3
extern const qint64_t __pow_data_Q_3[] attribute_hidden;
#define Q_3 __pow_data_Q_3

#endif
