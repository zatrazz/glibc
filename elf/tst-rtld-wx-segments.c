/* Test the glibc.rtld.wx_segments tunable.
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

#include <dlfcn.h>
#include <getopt.h>
#include <link.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/capture_subprocess.h>
#include <support/check.h>
#include <support/subprocess.h>
#include <support/support.h>
#include <support/xdlfcn.h>

#define MODULE "tst-rtld-wx-segments-mod.so"

static int restart;
#define CMDLINE_OPTIONS \
  { "restart", no_argument, &restart, 1 },

struct wx_objects
{
  /* Number of loaded objects, other than MODULE, with a writable and
     executable segment.  */
  int count;
  /* Whether MODULE has a writable and executable segment.  */
  bool module;
};

static int
find_wx_objects (struct dl_phdr_info *info, size_t size, void *closure)
{
  struct wx_objects *objs = closure;

  for (ElfW(Half) i = 0; i < info->dlpi_phnum; i++)
    if (info->dlpi_phdr[i].p_type == PT_LOAD
	&& (info->dlpi_phdr[i].p_flags & (PF_W | PF_X)) == (PF_W | PF_X))
      {
	if (strstr (info->dlpi_name, MODULE) != NULL)
	  objs->module = true;
	else
	  {
	    printf ("info: %s has a writable and executable segment\n",
		    info->dlpi_name[0] != '\0'
		    ? info->dlpi_name : "<main program>");
	    objs->count++;
	  }
	break;
      }

  return 0;
}

/* Run with glibc.rtld.wx_segments=0.  */
static int
handle_restart (void)
{
  void *h = dlopen (MODULE, RTLD_NOW);
  TEST_VERIFY (h == NULL);
  const char *err = dlerror ();
  TEST_VERIFY_EXIT (err != NULL);
  printf ("info: dlopen error: %s\n", err);
  TEST_VERIFY (strstr (err, "writable and executable segments are not allowed")
	       != NULL);
  return 0;
}

static int
do_test (void)
{
  if (restart)
    return handle_restart ();

  /* The module is loaded by default.  */
  void *h = xdlopen (MODULE, RTLD_NOW);
  int (*value) (void) = xdlsym (h, "tst_rtld_wx_segments_value");
  TEST_COMPARE (value (), 42);

  struct wx_objects objs = { 0, false };
  dl_iterate_phdr (find_wx_objects, &objs);
  TEST_VERIFY (objs.module);
  xdlclose (h);

  /* Some ABIs, such as HPPA and SPARC, place the PLT in a writable and
     executable segment, so the respawned test would not start.  */
  if (objs.count != 0)
    FAIL_UNSUPPORTED ("loaded objects have writable and executable segments");

  /* Run the test again with the tunable set, under the built loader.  */
  char *program = xasprintf ("%s/elf/tst-rtld-wx-segments",
			     support_objdir_root);
  char *args[] =
    { program, (char *) "--direct", (char *) "--restart", NULL };
  char *env[] = { (char *) "GLIBC_TUNABLES=glibc.rtld.wx_segments=0", NULL };
  struct support_spawn_wrapped *w = support_spawn_wrap (program, args, env, 0);
  struct support_capture_subprocess result
    = support_capture_subprogram (w->path, w->argv, w->envp);
  support_spawn_wrapped_free (w);
  support_capture_subprocess_check (&result, "tst-rtld-wx-segments", 0,
				    sc_allow_stdout);
  fputs (result.out.buffer, stdout);
  support_capture_subprocess_free (&result);
  free (program);

  return 0;
}

#include <support/test-driver.c>
