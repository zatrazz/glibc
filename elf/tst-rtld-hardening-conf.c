/* Test the dynamic loader hardening tunables set in /etc/tunables.conf.
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
#include <link.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/auxv.h>
#include <unistd.h>
#include <support/capture_subprocess.h>
#include <support/check.h>
#include <support/support.h>
#include <support/xstdio.h>
#include <support/xunistd.h>

/* The programs and shared objects used by the test are in the build
   directory, which test-container makes available at the same path.  */

struct object_counts
{
  /* Objects with a writable and executable segment.  */
  int wx;
  /* Objects that may require an executable stack.  */
  int execstack;
};

static int
count_objects (struct dl_phdr_info *info, size_t size, void *closure)
{
  struct object_counts *counts = closure;
  const char *name = info->dlpi_name[0] != '\0'
		     ? info->dlpi_name : "<main program>";
  ElfW(Addr) vdso = getauxval (AT_SYSINFO_EHDR);
  bool is_vdso = false;
  bool wx = false;
  bool execstack = true;

  for (ElfW(Half) i = 0; i < info->dlpi_phnum; i++)
    {
      const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
      if (ph->p_type == PT_LOAD)
	{
	  ElfW(Addr) start = info->dlpi_addr + ph->p_vaddr;
	  if (vdso != 0 && vdso >= start && vdso - start < ph->p_memsz)
	    is_vdso = true;
	  if ((ph->p_flags & (PF_W | PF_X)) == (PF_W | PF_X))
	    wx = true;
	}
      else if (ph->p_type == PT_GNU_STACK)
	execstack = (ph->p_flags & PF_X) != 0;
    }

  if (wx)
    {
      printf ("info: %s has a writable and executable segment\n", name);
      counts->wx++;
    }
  /* Without PT_GNU_STACK, the stack permissions depend on the ABI.  */
  if (execstack && !is_vdso)
    {
      printf ("info: %s may require an executable stack\n", name);
      counts->execstack++;
    }

  return 0;
}

static void
run_ldconfig (void *closure)
{
  char *prog = xasprintf ("%s/ldconfig", support_install_rootsbindir);
  char *args[] = { prog, NULL };

  execv (args[0], args);
  FAIL_EXIT1 ("execv: %m");
}

static void
check_program (const char *program, const char *module, int status,
	       const char *message)
{
  char *path = xasprintf ("%s/elf/%s", support_objdir_root, program);
  char *modpath = NULL;
  if (module != NULL)
    modpath = xasprintf ("%s/elf/%s", support_objdir_root, module);
  char *argv[] = { path, modpath, NULL };

  printf ("info: running %s %s\n", program, module != NULL ? module : "");
  struct support_capture_subprocess result
    = support_capture_subprogram (path, argv, NULL);
  if (message == NULL)
    support_capture_subprocess_check (&result, program, 0, sc_allow_stdout);
  else
    {
      support_capture_subprocess_check (&result, program, status,
					sc_allow_stdout | sc_allow_stderr);
      if (strstr (result.out.buffer, message) == NULL
	  && strstr (result.err.buffer, message) == NULL)
	FAIL ("%s: message \"%s\" not found\nstdout: %s\nstderr: %s",
	      program, message, result.out.buffer, result.err.buffer);
    }

  support_capture_subprocess_free (&result);
  free (modpath);
  free (path);
}

/* Whether glibc.rtld.wx_segments=0 and glibc.rtld.execstack=0 are set
   and checked.  */
static bool check_wx;
static bool check_execstack;

/* Write the settings to /etc/tunables.conf and run ldconfig to add them
   to the cache.  */
static void
do_prepare (int argc, char **argv)
{
  /* Some ABIs, such as HPPA and SPARC, place the PLT in a writable and
     executable segment, so glibc.rtld.wx_segments=0 would reject every
     program.  Likewise, glibc.rtld.execstack=0 would reject every program
     if the objects require an executable stack.  */
  struct object_counts counts = { 0, 0 };
  dl_iterate_phdr (count_objects, &counts);
  check_wx = counts.wx == 0;
  check_execstack = EXECSTACK_SUPPORTED && counts.execstack == 0;

  /* Non-overridable settings that also apply to AT_SECURE processes.  */
  char *conf = xasprintf ("%s/tunables.conf", support_sysconfdir_prefix);
  FILE *fp = xfopen (conf, "w");
  TEST_VERIFY_EXIT (fputs ("-*glibc.rtld.require_pie=1\n"
			   "-*glibc.rtld.textrel=0\n"
			   "-*glibc.rtld.bind_now=1\n"
			   "-*glibc.rtld.relro=1\n", fp) >= 0);
  if (check_wx)
    TEST_VERIFY_EXIT (fputs ("-*glibc.rtld.wx_segments=0\n", fp) >= 0);
  if (check_execstack)
    TEST_VERIFY_EXIT (fputs ("-*glibc.rtld.execstack=0\n", fp) >= 0);
  xfclose (fp);
  free (conf);

  xmkdirp ("/var/cache/ldconfig", 0777);
  struct support_capture_subprocess result
    = support_capture_subprocess (run_ldconfig, NULL);
  support_capture_subprocess_check (&result, "ldconfig", 0, sc_allow_none);
  support_capture_subprocess_free (&result);
}

static int
do_test (void)
{
  /* The environment can not override the settings, and setting
     glibc.rtld.enable_secure runs the programs as AT_SECURE.  */
  static const char *const tunables[] =
    {
      "glibc.rtld.require_pie=0:glibc.rtld.textrel=1"
      ":glibc.rtld.wx_segments=1:glibc.rtld.bind_now=0"
      ":glibc.rtld.execstack=1:glibc.rtld.relro=0",
      "glibc.rtld.enable_secure=1",
    };
  for (size_t i = 0; i < array_length (tunables); i++)
    {
      printf ("info: GLIBC_TUNABLES=%s\n", tunables[i]);
      setenv ("GLIBC_TUNABLES", tunables[i], 1);

      check_program ("tst-rtld-hardening-conf-helper", NULL, 0, NULL);

      check_program ("tst-rtld-require-pie-nopie", NULL, 127,
		     "Fatal glibc error: non-PIE executable is not allowed");

      check_program ("tst-rtld-hardening-conf-helper",
		     "tst-rtld-bind-now-mod.so", 1,
		     "undefined symbol: tst_rtld_bind_now_undefined");

#if TEXTREL_SUPPORTED
      check_program ("tst-rtld-hardening-conf-helper-textrel", NULL, 127,
		     "Fatal glibc error: text relocations are not allowed");
      check_program ("tst-rtld-hardening-conf-helper",
		     "tst-rtld-textrel-mod.so", 1,
		     "text relocations are not allowed");
#endif

      if (check_wx)
	{
	  check_program ("tst-rtld-hardening-conf-helper-wx", NULL, 127,
			 "Fatal glibc error: writable and executable segments"
			 " are not allowed");
	  check_program ("tst-rtld-hardening-conf-helper",
			 "tst-rtld-wx-segments-mod.so", 1,
			 "writable and executable segments are not allowed");
	}

      if (check_execstack)
	check_program ("tst-rtld-hardening-conf-helper-execstack", NULL, 127,
		       "Fatal glibc error: executable stack is not allowed");

      check_program ("tst-rtld-hardening-conf-helper-norelro", NULL, 127,
		     "Fatal glibc error: RELRO is required");
      check_program ("tst-rtld-hardening-conf-helper",
		     "tst-rtld-relro-mod-norelro.so", 1,
		     ": RELRO is required");
    }

  return 0;
}

#define PREPARE do_prepare
#include <support/test-driver.c>
