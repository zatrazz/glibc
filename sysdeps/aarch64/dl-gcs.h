/* Internal AArch64 GCS definitions.
   Copyright (C) 2026 Free Software Foundation, Inc.

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

#ifndef _DL_GCS_H
#define _DL_GCS_H

#include <verify.h>

typedef enum
{
  /* GCS is disabled.  */
  AARCH64_GCS_POLICY_DISABLED = 0,
  /* Enable GCS, abort if unmarked binary is found.  */
  AARCH64_GCS_POLICY_ENFORCED = 1,
  /* Optionally enable GCS if all startup dependencies are marked.  */
  AARCH64_GCS_POLICY_OPTIONAL = 2,
  /* Override binary marking and always enabled GCS.  */
  AARCH64_GCS_POLICY_OVERRIDE = 3
} aarch64_gcs_mode;

/* dl-start.S assumes aarch64_gcs_mode is representable as uint32_t.  */
verify (sizeof (aarch64_gcs_mode) == 4);

#endif
