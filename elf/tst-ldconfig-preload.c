/* Test the system-wide preload list stored in ld.so.cache.
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

/* ldconfig compiles /etc/ld.so.preload into ld.so.cache and the loader
   only consults the cache.  The test checks that objects listed in the
   file, directly or through 'include' directives, are preloaded in file
   order (with the files matched by a pattern in sorted name order) once
   ldconfig has run, that removing the file has no effect until ldconfig
   runs again, and that they then are no longer preloaded.  */

#include <dlfcn.h>
#include <getopt.h>
#include <link.h>
#include <stdlib.h>
#include <unistd.h>

#include <support/capture_subprocess.h>
#include <support/check.h>
#include <support/support.h>
#include <support/temp_file.h>
#include <support/xdlfcn.h>
#include <support/xunistd.h>

#define MOD_BUILT	"tst-dl-path-buf-mod.so"
#define MOD_DIR		"/tst-ldconfig-preload"
/* Two copies of the same module, so both can be loaded at once.  */
#define MOD_A		MOD_DIR "/libtst-ldconfig-preload-a.so"
#define MOD_B		MOD_DIR "/libtst-ldconfig-preload-b.so"

/* Options for the re-executed child.  */
static int restart;
static int expect_a;
static int expect_b;
static int expect_b_first;
#define CMDLINE_OPTIONS \
  { "restart", no_argument, &restart, 1 }, \
  { "expect-a", no_argument, &expect_a, 1 }, \
  { "expect-b", no_argument, &expect_b, 1 }, \
  { "expect-b-first", no_argument, &expect_b_first, 1 },

static void
run_ldconfig (void *closure __attribute__ ((unused)))
{
  char *prog = xasprintf ("%s/ldconfig", support_install_rootsbindir);
  char *args[] = { prog, NULL };
  execv (args[0], args);
  FAIL_EXIT1 ("execv (%s): %m", prog);
}

static void
ldconfig (void)
{
  struct support_capture_subprocess result
    = support_capture_subprocess (run_ldconfig, NULL);
  support_capture_subprocess_check (&result, "ldconfig", 0, sc_allow_none);
  support_capture_subprocess_free (&result);
}

enum { CHECK_A = 1, CHECK_B = 2, CHECK_B_FIRST = 4 };

/* Re-execute the test and check which modules the loader preloaded.  */
static void
check_child (const char *argv0, const char *what, int expect)
{
  char *spargv[7];
  int n = 0;
  spargv[n++] = (char *) argv0;
  spargv[n++] = (char *) "--direct";
  spargv[n++] = (char *) "--restart";
  if (expect & CHECK_A)
    spargv[n++] = (char *) "--expect-a";
  if (expect & CHECK_B)
    spargv[n++] = (char *) "--expect-b";
  if (expect & CHECK_B_FIRST)
    spargv[n++] = (char *) "--expect-b-first";
  spargv[n] = NULL;

  struct support_capture_subprocess result
    = support_capture_subprogram (spargv[0], spargv, NULL);
  support_capture_subprocess_check (&result, what, 0, sc_allow_stdout);
  support_capture_subprocess_free (&result);
}

/* Return the link map of the preloaded module PATH, or NULL.  */
static struct link_map *
preloaded (const char *path)
{
  void *h = dlopen (path, RTLD_NOW | RTLD_NOLOAD);
  if (h == NULL)
    return NULL;
  struct link_map *l;
  TEST_COMPARE (dlinfo (h, RTLD_DI_LINKMAP, &l), 0);
  xdlclose (h);
  return l;
}

static int
child (void)
{
  struct link_map *a = preloaded (MOD_A);
  struct link_map *b = preloaded (MOD_B);
  TEST_COMPARE (a != NULL, expect_a);
  TEST_COMPARE (b != NULL, expect_b);

  if (a != NULL && b != NULL)
    {
      /* Preloads are added to the namespace list in load order.  */
      bool b_first = false;
      for (struct link_map *l = b->l_next; l != NULL; l = l->l_next)
	if (l == a)
	  b_first = true;
      TEST_COMPARE (b_first, expect_b_first);
    }

  return 0;
}

static void
do_prepare (int argc, char **argv)
{
  if (restart)
    return;

  xmkdirp (MOD_DIR, 0777);
  add_temp_file (MOD_DIR);
  char *src = xasprintf ("%s/elf/" MOD_BUILT, support_objdir_root);
  support_copy_file (src, MOD_A);
  add_temp_file (MOD_A);
  support_copy_file (src, MOD_B);
  add_temp_file (MOD_B);
  free (src);

  xmkdirp ("/var/cache/ldconfig", 0777);
}
#define PREPARE do_prepare

static int
do_test (int argc, char **argv)
{
  if (restart)
    return child ();

  char *preload = xasprintf ("%s/ld.so.preload", support_sysconfdir_prefix);
  char *preload_d = xasprintf ("%s/ld.so.preload.d",
			       support_sysconfdir_prefix);
  char *preload_d_b = xasprintf ("%s/10-b.conf", preload_d);
  char *preload_d_ignored = xasprintf ("%s/20-a.ignored", preload_d);
  xmkdirp (preload_d, 0777);

  /* Comments are accepted, as with the file.  */
  support_write_file_string (preload,
			     "# system-wide preload\n"
			     " " MOD_A " # comment\n");
  ldconfig ();
  check_child (argv[0], "plain", CHECK_A);

  /* The loader does not read the file itself.  */
  xunlink (preload);
  check_child (argv[0], "file removed", CHECK_A);

  ldconfig ();
  check_child (argv[0], "file removed, ldconfig", 0);

  /* Both separators are accepted, as with the file.  */
  support_write_file_string (preload, MOD_B ":" MOD_A "\n");
  ldconfig ();
  check_child (argv[0], "colon", CHECK_A | CHECK_B | CHECK_B_FIRST);

  /* An absolute 'include' pattern is expanded in place, so its
     entries come before those on later lines.  */
  support_write_file_string (preload_d_b, MOD_B "\n");
  char *contents = xasprintf ("include %s/*.conf\n" MOD_A "\n", preload_d);
  support_write_file_string (preload, contents);
  free (contents);
  ldconfig ();
  check_child (argv[0], "absolute include",
	       CHECK_A | CHECK_B | CHECK_B_FIRST);

  /* A relative pattern is relative to the directory of the including
     file.  Files that do not match the pattern are ignored.  */
  support_write_file_string (preload,
			     MOD_A "\n"
			     "include ld.so.preload.d/*.conf\n");
  support_write_file_string (preload_d_ignored, MOD_A "\n");
  ldconfig ();
  check_child (argv[0], "relative include", CHECK_A | CHECK_B);
  xunlink (preload_d_b);
  xunlink (preload_d_ignored);

  /* Files matched by a pattern are read in sorted file name order, not
     in directory order.  The file for MOD_A is always created first,
     so the two cases below disagree if the creation or directory order
     is used.  */
  support_write_file_string (preload, "include ld.so.preload.d/*.conf\n");
  {
    char *a = xasprintf ("%s/20-a.conf", preload_d);
    char *b = xasprintf ("%s/10-b.conf", preload_d);
    support_write_file_string (a, MOD_A "\n");
    support_write_file_string (b, MOD_B "\n");
    ldconfig ();
    check_child (argv[0], "sorted include, b first",
		 CHECK_A | CHECK_B | CHECK_B_FIRST);
    xunlink (a);
    xunlink (b);
    free (a);
    free (b);
  }
  {
    char *a = xasprintf ("%s/10-a.conf", preload_d);
    char *b = xasprintf ("%s/20-b.conf", preload_d);
    support_write_file_string (a, MOD_A "\n");
    support_write_file_string (b, MOD_B "\n");
    ldconfig ();
    check_child (argv[0], "sorted include, a first", CHECK_A | CHECK_B);
    xunlink (a);
    xunlink (b);
    free (a);
    free (b);
  }

  /* An 'include' that matches nothing adds no entries.  */
  support_write_file_string (preload, "include /nonexistent/*.conf\n");
  ldconfig ();
  check_child (argv[0], "empty include", 0);

  xunlink (preload);
  TEST_COMPARE (rmdir (preload_d), 0);
  ldconfig ();
  check_child (argv[0], "cleanup", 0);

  free (preload_d_ignored);
  free (preload_d_b);
  free (preload_d);
  free (preload);
  return 0;
}

#define TEST_FUNCTION_ARGV do_test
#include <support/test-driver.c>
