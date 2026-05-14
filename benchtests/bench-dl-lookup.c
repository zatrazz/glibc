/* Benchmark dynamic linker symbol lookup with many DSOs.
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

/* This benchmark exercises the dynamic linker's symbol resolution
   path under loads where the main search list is large enough to
   trigger the fastload position-skip table (BZ#16709).

   Two scenarios are timed:

   * lookup_present — repeatedly call dlsym() to look up a symbol
     defined in this benchmark binary.  Each lookup walks the global
     scope and ultimately resolves to an early position; this
     measures the lookup hit path.

   * lookup_absent — repeatedly call dlsym() for a symbol that does
     not exist.  Each call walks the entire global scope before
     failing; this measures the lookup miss path, where the fastload
     table's "skip past everything" return value pays off most.

   To make the global scope large, the benchmark dlopen()s every
   bench-dl-lookup-modN.so module before timing starts.  The modules
   define unique symbols (fnN_M) so they all contribute to the
   relocation work, but none of those symbols are referenced from this
   binary — making them representative of the "5000 NEEDED libraries
   nobody references" workload from BZ#16709.

   To compare with the fastload disabled, run the bench through ld.so
   manually:

     GLIBC_TUNABLES=glibc.rtld.lookup_hash_cutoff=-1 \\
       ${BUILD}/elf/ld.so --library-path ... \\
       ./bench-dl-lookup

   …or build the bench, then run with and without the tunable set.  */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bench-timing.h"
#include "json-lib.h"

#define NUM_MODULES        100
#define LOOKUPS_PER_RUN    50000

/* Defined locally so lookup_present can find a symbol in the main
   program.  */
int bench_dl_lookup_present_target = 42;

/* These are exported names from the bench-dl-lookup-modN.so series.
   We do not call them from C; they exist solely to provide named
   symbols in the search list.  */

static void
do_bench (json_ctx_t *ctx, const char *variant_name,
          const char *lookup_name, int should_find)
{
  timing_t start, stop, diff;
  uint64_t total = 0;

  json_attr_object_begin (ctx, variant_name);

  TIMING_NOW (start);
  for (int i = 0; i < LOOKUPS_PER_RUN; ++i)
    {
      void *p = dlsym (RTLD_DEFAULT, lookup_name);
      (void) p;
      if (should_find && p == NULL)
        {
          fprintf (stderr, "bench-dl-lookup: %s unexpectedly not found\n",
                   lookup_name);
          exit (1);
        }
      if (!should_find && p != NULL)
        {
          fprintf (stderr, "bench-dl-lookup: %s unexpectedly found\n",
                   lookup_name);
          exit (1);
        }
    }
  TIMING_NOW (stop);
  TIMING_DIFF (diff, start, stop);
  TIMING_ACCUM (total, diff);

  json_attr_double (ctx, "duration", total);
  json_attr_double (ctx, "iterations", (double) LOOKUPS_PER_RUN);
  json_attr_double (ctx, "mean", (double) total / LOOKUPS_PER_RUN);
  json_attr_object_end (ctx);
}

int
main (int argc, char **argv)
{
  /* Pull in all the bench modules so they expand the global scope.
     This emulates a process that links against many shared libraries
     and triggers BZ#16709-style scope walks during symbol lookup.  */
  int loaded = 0;
  for (int i = 0; i < NUM_MODULES; ++i)
    {
      char path[80];
      snprintf (path, sizeof path, "bench-dl-lookup-mod%d.so", i);
      void *h = dlopen (path, RTLD_NOW | RTLD_GLOBAL);
      if (h == NULL)
        {
          /* The modules may not have been built (e.g. when the
             benchtest is invoked outside of the build tree).  Don't
             fail — just run a smaller workload.  */
          if (i == 0)
            {
              fprintf (stderr,
                       "bench-dl-lookup: no modules found (%s)\n",
                       dlerror ());
              break;
            }
          break;
        }
      ++loaded;
    }

  json_ctx_t json_ctx;
  json_init (&json_ctx, 0, stdout);
  json_document_begin (&json_ctx);
  json_attr_string (&json_ctx, "timing_type", TIMING_TYPE);
  json_attr_object_begin (&json_ctx, "functions");
  json_attr_object_begin (&json_ctx, "dl_lookup");
  json_attr_int (&json_ctx, "modules_loaded", loaded);

  /* Hit path: symbol defined in the main program — found quickly.  */
  do_bench (&json_ctx, "lookup_present_main",
            "bench_dl_lookup_present_target", 1);

  /* Hit path: symbol defined deep in libc (so the scope walk has to
     pass through every loaded module first).  */
  do_bench (&json_ctx, "lookup_present_libc", "printf", 1);

  /* Miss path: symbol that does not exist.  Without fastload, every
     such lookup walks the entire scope.  With fastload, the lookup is
     short-circuited at the first call.  */
  do_bench (&json_ctx, "lookup_absent",
            "bench_dl_lookup_definitely_not_a_symbol_42xyz", 0);

  json_attr_object_end (&json_ctx);
  json_attr_object_end (&json_ctx);
  json_document_end (&json_ctx);

  return 0;
}
