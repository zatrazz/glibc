/* Measure fnmatch function.
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

#define TEST_MAIN
#define TEST_NAME "fnmatch"

#include <fnmatch.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "json-lib.h"
#include "bench-timing.h"

#define LOOP_ITERS 200000

struct fnm_case
{
  const char *name;       /* Human-readable case label.  */
  const char *pattern;
  const char *string;
  int flags;
};

/* ASCII-only cases (work in any locale; exercise the single-byte
   fast path under MB_CUR_MAX == 1).  */
static const struct fnm_case ascii_cases[] =
  {
    { "ascii-literal-match",
      "abcdefghij", "abcdefghij", 0 },
    { "ascii-literal-nomatch",
      "abcdefghij", "abcdefghik", 0 },
    { "ascii-q-match",
      "a?c?e?g?i?", "abcdefghij", 0 },
    { "ascii-star-prefix",
      "abc*", "abcdefghijklmnopqrstuvwxyz", 0 },
    { "ascii-star-middle",
      "abc*xyz", "abcdefghijklmnopqrstuvwxyz", 0 },
    { "ascii-star-backtrack",
      "a*b*c*d*e*f*g", "abcdabcdabcdabcdabcdabcdg", 0 },
    { "ascii-bracket-class",
      "[a-z][a-z][a-z][a-z]", "abcd", 0 },
    { "ascii-bracket-negated",
      "[!0-9][!0-9][!0-9][!0-9]", "abcd", 0 },
    { "ascii-charclass",
      "[[:alpha:]][[:alpha:]][[:alpha:]][[:digit:]]", "abc1", 0 },
    { "ascii-pathname",
      "/usr/*/bin/*", "/usr/local/bin/ls", FNM_PATHNAME },
    { "ascii-pathname-nomatch",
      "/usr/*/bin/*", "/usr/local/lib/ls", FNM_PATHNAME },
    { "ascii-long-star",
      "*xyz", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaxyz", 0 },
  };

/* Multibyte (UTF-8) cases.  */
static const struct fnm_case utf8_cases[] =
  {
    /* "café" matched by "café".  */
    { "utf8-literal-match",
      "caf\xc3\xa9", "caf\xc3\xa9", 0 },
    { "utf8-literal-nomatch",
      "caf\xc3\xa9", "caf\xc3\xa8", 0 },
    /* "?" must match exactly one logical character.  */
    { "utf8-q-match",
      "??", "\xc3\xa9\xc3\xa9", 0 },
    { "utf8-q-nomatch-single",
      "??", "\xc3\xa9", 0 },
    /* "*" with multibyte tail.  */
    { "utf8-star-tail",
      "*\xc3\xa9", "hello world caf\xc3\xa9", 0 },
    { "utf8-bracket-range",
      "[\xc3\xa1-\xc3\xaa]", "\xc3\xa9", 0 },
    { "utf8-bracket-range-nomatch",
      "[\xc3\xa1-\xc3\xa4]", "\xc3\xa9", 0 },
    /* Mixed multibyte and ASCII.  */
    { "utf8-mixed",
      "h?llo \xc3\xa9*\xc3\xa9",
      "hello \xc3\xa9world\xc3\xa9", 0 },
    /* Pure ASCII pattern/string but the locale uses multibyte
       (exercises the >= 0x80 multibyte path even when content is
       ASCII).  */
    { "utf8-locale-ascii-content",
      "a*b*c*d*e*f*g", "abcdabcdabcdabcdabcdabcdg", 0 },
  };

static void
do_one_case (json_ctx_t *json_ctx, const char *variant,
             const struct fnm_case *c)
{
  timing_t start, stop, cur;
  volatile int res;
  size_t i;

  json_element_object_begin (json_ctx);
  json_attr_string (json_ctx, "case", c->name);
  json_attr_string (json_ctx, "variant", variant);
  json_attr_string (json_ctx, "pattern", c->pattern);
  json_attr_uint (json_ctx, "pattern-len", strlen (c->pattern));
  json_attr_uint (json_ctx, "string-len", strlen (c->string));
  json_attr_uint (json_ctx, "flags", c->flags);

  TIMING_NOW (start);
  for (i = 0; i < LOOP_ITERS; ++i)
    res = fnmatch (c->pattern, c->string, c->flags);
  TIMING_NOW (stop);
  (void) res;

  TIMING_DIFF (cur, start, stop);
  json_attr_double (json_ctx, "timing",
                    (double) cur / (double) LOOP_ITERS);
  json_element_object_end (json_ctx);
}

int
do_test (void)
{
  json_ctx_t json_ctx;

  json_init (&json_ctx, 0, stdout);

  json_document_begin (&json_ctx);
  json_attr_string (&json_ctx, "timing_type", TIMING_TYPE);
  json_attr_object_begin (&json_ctx, "functions");
  json_attr_object_begin (&json_ctx, TEST_NAME);
  json_attr_string (&json_ctx, "bench-variant", "default");
  json_array_begin (&json_ctx, "results");

  /* Run ASCII benchmarks under the C locale (MB_CUR_MAX == 1).  */
  if (setlocale (LC_ALL, "C") == NULL)
    {
      fprintf (stderr, "fatal: cannot set C locale\n");
      return 1;
    }
  for (size_t i = 0; i < sizeof (ascii_cases) / sizeof (ascii_cases[0]); ++i)
    do_one_case (&json_ctx, "C", &ascii_cases[i]);

  /* Run UTF-8 benchmarks under en_US.UTF-8 (MB_CUR_MAX > 1).  */
  if (setlocale (LC_ALL, "en_US.UTF-8") == NULL)
    {
      fprintf (stderr, "warning: cannot set en_US.UTF-8 locale, "
                       "skipping multibyte benchmarks\n");
    }
  else
    {
      for (size_t i = 0;
           i < sizeof (utf8_cases) / sizeof (utf8_cases[0]); ++i)
        do_one_case (&json_ctx, "en_US.UTF-8", &utf8_cases[i]);

      /* Also run the ASCII cases in a UTF-8 locale to measure the
         overhead (if any) of MB_CUR_MAX > 1 on ASCII input.  */
      for (size_t i = 0;
           i < sizeof (ascii_cases) / sizeof (ascii_cases[0]); ++i)
        do_one_case (&json_ctx, "en_US.UTF-8-ascii-input",
                     &ascii_cases[i]);
    }

  json_array_end (&json_ctx);
  json_attr_object_end (&json_ctx);
  json_attr_object_end (&json_ctx);
  json_document_end (&json_ctx);

  return 0;
}

#include <support/test-driver.c>
