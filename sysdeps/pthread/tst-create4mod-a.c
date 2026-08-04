/* DSO for tst-create4 whose constructor blocks on a barrier (BZ 15686).
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

#include <stdatomic.h>

#include "tst-create4.h"

static void __attribute__ ((constructor))
init_a (void)
{
  atomic_store_explicit (&tst_create4_ctor_running, 1,
                         memory_order_release);
  pthread_barrier_wait (&tst_create4_ctor_barrier);
}
