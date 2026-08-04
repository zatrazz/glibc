/* Shared definitions for tst-create4 and its modules.
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

#ifndef _TST_CREATE4_H
#define _TST_CREATE4_H

#include <pthread.h>
#include <stdatomic.h>

/* Barrier that the module constructor waits on.  Main signals it after
   the concurrent dlopen of the unrelated module completes.  Defined in
   the main executable and exported to the modules via the dynamic
   symbol table (-Wl,-export-dynamic).  */
extern pthread_barrier_t tst_create4_ctor_barrier;

/* Flag set to 1 by the module constructor once it has started (i.e.
   after dl_load_lock has been released for the constructor
   execution).  Main spins on this flag to guarantee the constructor
   is executing before it creates the second worker thread.  */
extern atomic_int tst_create4_ctor_running;

#endif
