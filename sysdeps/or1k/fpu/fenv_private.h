/* Private floating point rounding and exceptions handling.  OpenRISC version.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.
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

#ifndef OR1K_FENV_PRIVATE_H
#define OR1K_FENV_PRIVATE_H 1

#include <fenv.h>
#include <fenv-impl.h>
#include <fpu_control.h>

#define libc_fesetround  libc_fesetround_or1k
#define libc_fesetroundf libc_fesetround_or1k
#define libc_fesetroundl libc_fesetround_or1k

static __always_inline void
libc_feholdexcept_setround_or1k (fenv_t *envp, int round)
{
  fpu_control_t cw;
  fpu_control_t cw_new;

  /* Get and store the environment.  */
  _FPU_GETCW (cw);
  *envp = cw;

  /* Clear the status flags and rounding mode.  */
  cw_new = cw & ~(FE_ALL_EXCEPT | _FPU_FPCSR_RM_MASK);

  /* Set rounding mode.  */
  cw_new |= round;

  if (cw != cw_new)
    _FPU_SETCW (cw_new);
}

#define libc_feholdexcept_setround  libc_feholdexcept_setround_or1k
#define libc_feholdexcept_setroundf libc_feholdexcept_setround_or1k
#define libc_feholdexcept_setroundl libc_feholdexcept_setround_or1k

#define libc_feupdateenv_test  libc_feupdateenv_test_or1k
#define libc_feupdateenv_testf libc_feupdateenv_test_or1k
#define libc_feupdateenv_testl libc_feupdateenv_test_or1k

static __always_inline void
libc_feholdsetround_or1k (fenv_t *envp, int round)
{
  fpu_control_t cw;

  _FPU_GETCW (cw);
  *envp = cw;

  /* Check whether rounding modes are different.  */
  round = (cw ^ round) & _FPU_FPCSR_RM_MASK;

  /* Set new rounding mode if different.  */
  if (__glibc_unlikely (round != 0))
    _FPU_SETCW (cw ^ round);
}

#define libc_feholdsetround  libc_feholdsetround_or1k
#define libc_feholdsetroundf libc_feholdsetround_or1k
#define libc_feholdsetroundl libc_feholdsetround_or1k

static __always_inline void
libc_feresetround_or1k (fenv_t *envp)
{
  fpu_control_t cw;
  int round;

  _FPU_GETCW (cw);

  /* Check whether rounding modes are different.  */
  round = (*envp ^ cw) & _FPU_FPCSR_RM_MASK;

  /* Restore the rounding mode if it was changed.  */
  if (__glibc_unlikely (round != 0))
    _FPU_SETCW (cw ^ round);
}

#define libc_feresetround  libc_feresetround_or1k
#define libc_feresetroundf libc_feresetround_or1k
#define libc_feresetroundl libc_feresetround_or1k

#include_next <fenv_private.h>

#endif
