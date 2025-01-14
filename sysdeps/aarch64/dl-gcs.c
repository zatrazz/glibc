/* AArch64 GCS functions.
   Copyright (C) 2024-2025 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <errno.h>
#include <libintl.h>
#include <ldsodefs.h>

static void
fail (struct link_map *l, const char *program)
{
  if (program && program[0])
    _dl_fatal_printf ("%s: %s: %s\n", program, l->l_name, "not GCS compatible");
  else if (program)
    _dl_fatal_printf ("%s\n", "not GCS compatible");
  else
    _dl_signal_error (0, l->l_name, "dlopen", "not GCS compatible");
}

static void
unsupported (void)
{
  _dl_fatal_printf ("%s\n", "unsupported GCS policy");
}

static void
check_gcs (struct link_map *l, const char *program)
{
#ifdef SHARED
  /* Ignore GCS marking on ld.so: its properties are not processed.  */
  if (is_rtld_link_map (l->l_real))
    return;
#endif
  bool for_dlopen = program == NULL;
  if (!l->l_mach.gcs)
    {
      if (GLRO(dl_aarch64_gcs_policy) == 2 || for_dlopen)
	fail (l, program);
      if (GLRO(dl_aarch64_gcs_policy) == 1)
	GL(dl_aarch64_gcs) = 0;
      else
	unsupported ();
    }
}

/* Apply GCS policy for L and its dependencies.
   PROGRAM is NULL when this check is invoked for dl_open.  */

void
_dl_gcs_check (struct link_map *l, const char *program)
{
  /* GCS is disabled.  */
  if (GL(dl_aarch64_gcs) == 0)
    return;
  /* GCS marking is ignored.  */
  if (GLRO(dl_aarch64_gcs_policy) == 0)
    return;

  check_gcs (l, program);
  for (unsigned int i = 0; i < l->l_searchlist.r_nlist; i++)
    check_gcs (l->l_searchlist.r_list[i], program);
}

/* Used to report error when prctl system call to enabled GCS fails.  */

void _dl_gcs_enable_failed (int code)
{
  _dl_fatal_printf ("failed to enable GCS: %d\n", -code);
}
