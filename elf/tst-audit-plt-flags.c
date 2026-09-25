/* Check that LA_SYMB_NOPLTENTER and LA_SYMB_NOPLTEXIT are honored per auditor.
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

#include <array_length.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/capture_subprocess.h>
#include <support/check.h>
#include <support/subprocess.h>
#include <support/support.h>

static int restart;
#define CMDLINE_OPTIONS \
  { "restart", no_argument, &restart, 1 },

int tst_audit_plt_flags_func1 (void);
int tst_audit_plt_flags_func2 (void);
int tst_audit_plt_flags_func3 (void);

static int
handle_restart (void)
{
  TEST_COMPARE (tst_audit_plt_flags_func1 (), 1);
  TEST_COMPARE (tst_audit_plt_flags_func2 (), 2);
  TEST_COMPARE (tst_audit_plt_flags_func3 (), 3);
  return 0;
}

static int
count_lines (const char *buf, const char *line)
{
  size_t len = strlen (line);
  int n = 0;
  for (const char *p = buf; p != NULL && *p != '\0'; )
    {
      if (strncmp (p, line, len) == 0 && p[len] == '\n')
	n++;
      p = strchr (p, '\n');
      if (p != NULL)
	p++;
    }
  return n;
}

static int
do_test (void)
{
  if (restart)
    return handle_restart ();

  char *audit = xasprintf ("%s/elf/tst-auditmod-plt-flags-a.so"
			   ":%s/elf/tst-auditmod-plt-flags-b.so",
			   support_objdir_root, support_objdir_root);
  setenv ("LD_AUDIT", audit, 1);
  free (audit);

  char *program = xasprintf ("%s/elf/tst-audit-plt-flags",
			     support_objdir_root);
  char *args[] = { program, (char *) "--direct", (char *) "--restart", NULL };
  struct support_spawn_wrapped *w
    = support_spawn_wrap (program, args, NULL, 0);
  struct support_capture_subprocess result
    = support_capture_subprogram (w->path, w->argv, w->envp);
  support_spawn_wrapped_free (w);
  free (program);
  support_capture_subprocess_check (&result, "tst-audit-plt-flags", 0,
				    sc_allow_stderr);
  const char *err = result.err.buffer;

  if (strstr (err, ": la_pltexit: ") == NULL)
    FAIL_UNSUPPORTED ("la_pltexit not supported");

  /* Both auditors request la_pltexit in la_pltenter.  The first one
     sets LA_SYMB_NOPLTEXIT for func1 and LA_SYMB_NOPLTENTER for func3,
     the second one sets LA_SYMB_NOPLTEXIT for func2.  */
  static const struct
  {
    const char *line;
    int count;
  } expected[] =
    {
      { "a: la_pltenter: tst_audit_plt_flags_func1", 1 },
      { "a: la_pltexit: tst_audit_plt_flags_func1", 0 },
      { "b: la_pltenter: tst_audit_plt_flags_func1", 1 },
      { "b: la_pltexit: tst_audit_plt_flags_func1", 1 },
      { "a: la_pltenter: tst_audit_plt_flags_func2", 1 },
      { "a: la_pltexit: tst_audit_plt_flags_func2", 1 },
      { "b: la_pltenter: tst_audit_plt_flags_func2", 1 },
      { "b: la_pltexit: tst_audit_plt_flags_func2", 0 },
      { "a: la_pltenter: tst_audit_plt_flags_func3", 0 },
      { "a: la_pltexit: tst_audit_plt_flags_func3", 1 },
      { "b: la_pltenter: tst_audit_plt_flags_func3", 1 },
      { "b: la_pltexit: tst_audit_plt_flags_func3", 1 },
    };
  for (size_t j = 0; j < array_length (expected); j++)
    if (count_lines (err, expected[j].line) != expected[j].count)
      FAIL ("expected %d \"%s\" lines", expected[j].count,
	    expected[j].line);

  if (support_record_failure_is_failed ())
    printf ("info: auditor output:\n%s", err);

  support_capture_subprocess_free (&result);
  return 0;
}

#include <support/test-driver.c>
