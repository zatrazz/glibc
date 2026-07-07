/* Measure the mpn routines from GNU MP indirectly, through the exported
   printf/strtod interfaces that use them internally.
   Copyright (C) 2026 Free Software Foundation, Inc.
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

/* The low-level natural-number (mpn) primitives from GNU MP
   (__mpn_mul_1, __mpn_addmul_1, __mpn_submul_1, __mpn_lshift, ...) are the
   hot inner loops of the multiprecision arithmetic used for
   correctly-rounded floating point <-> string conversion:
   stdio-common/printf_fp.c (printf %f/%e/%g) and stdlib/strtod_l.c (strtod).
   Several architectures provide hand-written assembly for them under
   sysdeps/.

   Rather than call those internal (attribute_hidden) symbols directly, this
   benchmark drives them through the exported converters and sweeps the knobs
   that scale the amount of mpn work performed:

     - printf "%.*f"/"%.*e"/"%.*g": the requested precision, which sets the
       number of generated digits (and hence limbs) directly;
     - printf "%f" of increasing magnitude / decreasing subnormals: the
       binary exponent, which sets how many limbs the fraction spans;
     - printf "%.*Lf": the same, in the wider long double format;
     - strtod/strtold of decimal strings with an increasing number of
       significant digits, which drives the exact big-integer comparison in
       the slow (correctly-rounded) parsing path.

   This measures the mpn implementation the build actually selected (assembly
   on x86_64, powerpc, ... ; generic C elsewhere) exactly as it is reached in
   production, including the surrounding conversion overhead.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bench-timing.h"
#include "json-lib.h"

/* 10^e for small integer e; avoids a libm dependency for the test.  */
static double
pow10i (int e)
{
  double r = 1.0;
  for (int i = 0; i < e; i++)
    r *= 10.0;
  for (int i = 0; i > e; i--)
    r /= 10.0;
  return r;
}

/* Total per-variant "work budget" (roughly chars produced/consumed); the
   iteration count is derived from it so cheap and expensive variants each
   run long enough to measure but the whole benchmark stays bounded.  */
#define WORK_BUDGET 8000000UL
#define MIN_ITERS   2000
#define MAX_ITERS   2000000
#define NREPS       5

/* Precision / significant-digit sweep shared by several groups.  */
static const int precs[] = { 0, 1, 6, 17, 50, 100, 300, 1000 };
#define NPREC (sizeof (precs) / sizeof (precs[0]))

/* Decimal exponents for the magnitude sweep of "%f".  */
static const int exps[] = { 0, 20, 50, 100, 200, 300, -50, -100, -300 };
#define NEXP (sizeof (exps) / sizeof (exps[0]))

static char buf[8192];
/* Volatile sink so the conversions cannot be optimised away.  */
static volatile unsigned long sink;

/* A value with a "hard" full-width mantissa so digit generation is not
   trivially short.  */
#define HARD_MANT 1.4142135623730951

static long
clamp_iters (unsigned long cost)
{
  long it = WORK_BUDGET / (cost + 16);
  if (it < MIN_ITERS)
    it = MIN_ITERS;
  if (it > MAX_ITERS)
    it = MAX_ITERS;
  return it;
}

/* Time THUNK(arg) over ITERS iterations, best of NREPS, in timing units.  */
#define TIME_BEST(best, iters, stmt)					\
  do {									\
    (best) = 1e30;							\
    for (int rep = 0; rep < NREPS; rep++)				\
      {									\
	timing_t start, stop, elapsed;					\
	TIMING_NOW (start);						\
	for (long it = 0; it < (iters); it++)				\
	  { stmt; }							\
	TIMING_NOW (stop);						\
	TIMING_DIFF (elapsed, start, stop);				\
	double per = (double) elapsed / (iters);			\
	if (per < (best))						\
	  (best) = per;							\
      }									\
  } while (0)

static json_ctx_t json_ctx;

static void
emit (const char *variant, long iters, double best, double work)
{
  json_attr_object_begin (&json_ctx, variant);
  json_attr_double (&json_ctx, "iterations", iters);
  json_attr_double (&json_ctx, "mean", best);
  /* "work" is the driving parameter (digits produced/consumed): a proxy for
     the number of limbs, so mean/work approximates time per limb.  */
  json_attr_double (&json_ctx, "mean-per-work", work > 0 ? best / work : best);
  json_attr_object_end (&json_ctx);
}

/* printf "%.*<conv>f" precision sweep of a hard-mantissa value.  */
static void
bench_printf_prec (const char *fname, char conv, double val)
{
  char fmt[8] = { '%', '.', '*', conv, 0 };
  json_attr_object_begin (&json_ctx, fname);
  fprintf (stderr, "\n%s (value=%g):\n", fname, val);
  for (unsigned i = 0; i < NPREC; i++)
    {
      int p = precs[i];
      long iters = clamp_iters (p + 20);
      double best;
      TIME_BEST (best, iters, sink += snprintf (buf, sizeof buf, fmt, p, val));
      int len = snprintf (buf, sizeof buf, fmt, p, val);
      char v[24];
      snprintf (v, sizeof v, "prec_%04d", p);
      emit (v, iters, best, len);
      fprintf (stderr, "  prec=%-4d len=%-5d %10.1f ticks/call\n", p, len, best);
    }
  json_attr_object_end (&json_ctx);
}

/* printf "%.20f" over a magnitude sweep (drives the fraction limb count).  */
static void
bench_printf_mag (void)
{
  json_attr_object_begin (&json_ctx, "snprintf_f_mag");
  fprintf (stderr, "\nsnprintf_f_mag (%%.20f, value=1.41e<exp>):\n");
  for (unsigned i = 0; i < NEXP; i++)
    {
      double val = HARD_MANT * pow10i (exps[i]);
      long iters = clamp_iters (abs (exps[i]) + 40);
      double best;
      TIME_BEST (best, iters, sink += snprintf (buf, sizeof buf, "%.20f", val));
      int len = snprintf (buf, sizeof buf, "%.20f", val);
      char v[24];
      snprintf (v, sizeof v, "exp_%+05d", exps[i]);
      emit (v, iters, best, len);
      fprintf (stderr, "  exp=%-5d len=%-5d %10.1f ticks/call\n",
	       exps[i], len, best);
    }
  json_attr_object_end (&json_ctx);
}

/* printf "%.*Lf" precision sweep of a long double.  */
static void
bench_printf_prec_ld (void)
{
  long double val = 1.4142135623730950488L;
  json_attr_object_begin (&json_ctx, "snprintf_Lf");
  fprintf (stderr, "\nsnprintf_Lf (%%.*Lf):\n");
  for (unsigned i = 0; i < NPREC; i++)
    {
      int p = precs[i];
      long iters = clamp_iters (p + 20);
      double best;
      TIME_BEST (best, iters, sink += snprintf (buf, sizeof buf, "%.*Lf", p, val));
      int len = snprintf (buf, sizeof buf, "%.*Lf", p, val);
      char v[24];
      snprintf (v, sizeof v, "prec_%04d", p);
      emit (v, iters, best, len);
      fprintf (stderr, "  prec=%-4d len=%-5d %10.1f ticks/call\n", p, len, best);
    }
  json_attr_object_end (&json_ctx);
}

/* Build a decimal string with N significant digits: "1.<N-1 random digits>".  */
static void
make_decimal (char *s, size_t sz, int ndigits, const char *suffix)
{
  static uint64_t r = 0x243f6a8885a308d3UL;
  size_t k = 0;
  s[k++] = '1';
  if (ndigits > 1)
    s[k++] = '.';
  for (int i = 1; i < ndigits && k < sz - 8; i++)
    {
      r ^= r << 13; r ^= r >> 7; r ^= r << 17;
      s[k++] = '0' + (r % 10);
    }
  strcpy (s + k, suffix);
}

/* strtod/strtold over an increasing number of significant digits.  */
static void
bench_strto (const char *fname, int longdouble, const char *suffix)
{
  json_attr_object_begin (&json_ctx, fname);
  fprintf (stderr, "\n%s (%d..%d sig. digits, suffix=\"%s\"):\n",
	   fname, precs[1], precs[NPREC - 1], suffix);
  /* Skip precs[0] == 0 (meaningless as a digit count; would also collide
     with precs[1] == 1 after clamping).  */
  for (unsigned i = 1; i < NPREC; i++)
    {
      int d = precs[i];
      char s[2048];
      make_decimal (s, sizeof s, d, suffix);
      long iters = clamp_iters (d + 20);
      double best;
      if (longdouble)
	TIME_BEST (best, iters, sink += (unsigned long) strtold (s, NULL));
      else
	TIME_BEST (best, iters, sink += (unsigned long) strtod (s, NULL));
      char v[24];
      snprintf (v, sizeof v, "digits_%04d", d);
      emit (v, iters, best, d);
      fprintf (stderr, "  digits=%-4d %10.1f ticks/call\n", d, best);
    }
  json_attr_object_end (&json_ctx);
}

int
main (void)
{
  json_init (&json_ctx, 0, stdout);
  json_document_begin (&json_ctx);
  json_attr_string (&json_ctx, "timing_type", TIMING_TYPE);
  json_attr_object_begin (&json_ctx, "functions");

  bench_printf_prec ("snprintf_f", 'f', HARD_MANT);
  bench_printf_prec ("snprintf_e", 'e', HARD_MANT);
  bench_printf_prec ("snprintf_g", 'g', HARD_MANT);
  bench_printf_mag ();
  bench_printf_prec_ld ();
  bench_strto ("strtod", 0, "");
  bench_strto ("strtold", 1, "");
  /* Subnormal / large-exponent parsing paths.  */
  bench_strto ("strtod_e300", 0, "e-300");

  json_attr_object_end (&json_ctx);
  json_document_end (&json_ctx);
  return 0;
}
