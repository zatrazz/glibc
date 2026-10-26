/* Test glibc.rtld.relro=2 with dlopen.
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
#include <stdlib.h>
#include <support/capture_subprocess.h>
#include <support/subprocess.h>
#include <support/support.h>
#include <sys/auxv.h>

#define RELRO_CHECK_MODULES_ONLY
#include "tst-rtld-relro.c"

static int restart;
#define CMDLINE_OPTIONS \
  { "restart", no_argument, &restart, 1 },

static ElfW(Addr) vdso;

/* Count the loaded objects, other than the vDSO (which the loader does not
   check), that are not linked with full RELRO.  */
static int
count_partial_relro (struct dl_phdr_info *info, size_t size, void *closure)
{
  int *count = closure;

  for (ElfW(Half) i = 0; i < info->dlpi_phnum; i++)
    {
      const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
      if (vdso != 0
	  && ph->p_type == PT_LOAD
	  && vdso >= info->dlpi_addr + ph->p_vaddr
	  && vdso - (info->dlpi_addr + ph->p_vaddr) < ph->p_memsz)
	return 0;
    }

  bool relro, now;
  get_relro_info (info, &relro, &now);
  if (!relro || !now)
    {
      printf ("info: %s is not linked with full RELRO\n",
	      info->dlpi_name[0] != '\0' ? info->dlpi_name : "<main program>");
      ++*count;
    }

  return 0;
}

static int
do_test (void)
{
  if (restart)
    {
      check_modules (2);
      return 0;
    }

  vdso = getauxval (AT_SYSINFO_EHDR);

  /* The respawned test can only start if every object it uses is linked
     with full RELRO, which depends on how glibc and the toolchain libraries
     (such as libgcc_s on ARM) were built.  */
  int count = 0;
  dl_iterate_phdr (count_partial_relro, &count);
  if (count != 0)
    FAIL_UNSUPPORTED ("loaded objects are not linked with full RELRO");

  /* Run the test again with the tunable set, under the built loader.  */
  char *program = xasprintf ("%s/elf/tst-rtld-relro-full",
			     support_objdir_root);
  char *args[] =
    {
      program, (char *) "--direct",
      (char *) "--restart",
      NULL
    };
  char *env[] =
    {
      (char *) "GLIBC_TUNABLES=glibc.rtld.relro=2",
      NULL
    };
  struct support_spawn_wrapped *w = support_spawn_wrap (program, args, env, 0);
  struct support_capture_subprocess result
    = support_capture_subprogram (w->path, w->argv, w->envp);
  support_spawn_wrapped_free (w);
  support_capture_subprocess_check (&result, "tst-rtld-relro-full", 0,
				    sc_allow_stdout);
  fputs (result.out.buffer, stdout);
  support_capture_subprocess_free (&result);
  free (program);

  return 0;
}

#include <support/test-driver.c>
