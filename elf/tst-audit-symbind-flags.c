/* Check that la_symbind and la_pltenter flags are per auditor.
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

#include <getopt.h>
#include <stdbool.h>
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

int tst_audit_symbind_flags_func1 (void);
int tst_audit_symbind_flags_func2 (void);

static int
handle_restart (void)
{
  /* Call each function twice so la_pltenter is also exercised after
     the lazy binding.  */
  TEST_COMPARE (tst_audit_symbind_flags_func1 (), 1);
  TEST_COMPARE (tst_audit_symbind_flags_func1 (), 1);
  TEST_COMPARE (tst_audit_symbind_flags_func2 (), 2);
  TEST_COMPARE (tst_audit_symbind_flags_func2 (), 2);
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

static void
run (char *program, bool bindnow)
{
  if (bindnow)
    setenv ("LD_BIND_NOW", "1", 1);
  else
    unsetenv ("LD_BIND_NOW");

  char *args[] = { program, (char *) "--direct", (char *) "--restart", NULL };
  struct support_spawn_wrapped *w
    = support_spawn_wrap (program, args, NULL, 0);
  struct support_capture_subprocess result
    = support_capture_subprogram (w->path, w->argv, w->envp);
  support_spawn_wrapped_free (w);
  support_capture_subprocess_check (&result, "tst-audit-symbind-flags", 0,
				    sc_allow_stderr);
  const char *err = result.err.buffer;

  /* tst-auditmod-symbind-flags-a.so sets LA_SYMB_NOPLTENTER and
     LA_SYMB_NOPLTEXIT for func1 in la_symbind, which must not be seen by
     tst-auditmod-symbind-flags-b.so.  ld.so sets both for bind-now.  */
  const char *expected1 = bindnow
    ? "b: la_symbind: tst_audit_symbind_flags_func1 3"
    : "b: la_symbind: tst_audit_symbind_flags_func1 0";
  const char *expected2 = bindnow
    ? "b: la_symbind: tst_audit_symbind_flags_func2 3"
    : "b: la_symbind: tst_audit_symbind_flags_func2 0";
  TEST_COMPARE (count_lines (err, expected1), 1);
  TEST_COMPARE (count_lines (err, expected2), 1);

  /* Not all architectures support la_pltenter.  If they do, the first auditor
     sets LA_SYMB_NOPLTENTER for func2 in its first la_pltenter, which must
     not stop the calls for the second auditor.  */
  if (count_lines (err, "a: la_pltenter: tst_audit_symbind_flags_func2") > 0)
    {
      TEST_VERIFY (!bindnow);
      TEST_COMPARE (count_lines (err,
		    "a: la_pltenter: tst_audit_symbind_flags_func2"), 1);
      TEST_COMPARE (count_lines (err,
		    "b: la_pltenter: tst_audit_symbind_flags_func1"), 2);
      TEST_COMPARE (count_lines (err,
		    "b: la_pltenter: tst_audit_symbind_flags_func2"), 2);
    }
  else if (!bindnow)
    puts ("info: la_pltenter not called, skipping its checks");

  if (support_record_failure_is_failed ())
    printf ("info: auditor output (LD_BIND_NOW=%d):\n%s", bindnow, err);

  support_capture_subprocess_free (&result);
}

static int
do_test (void)
{
  if (restart)
    return handle_restart ();

  char *audit = xasprintf ("%s/elf/tst-auditmod-symbind-flags-a.so"
			   ":%s/elf/tst-auditmod-symbind-flags-b.so",
			   support_objdir_root, support_objdir_root);
  setenv ("LD_AUDIT", audit, 1);
  free (audit);

  char *program = xasprintf ("%s/elf/tst-audit-symbind-flags",
			     support_objdir_root);
  run (program, false);
  run (program, true);
  free (program);

  return 0;
}

#include <support/test-driver.c>
