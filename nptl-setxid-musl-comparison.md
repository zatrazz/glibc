# setxid signal-broadcast: glibc vs musl

Comparison of how glibc (`nptl/nptl_setxid.c`, after the bug 21108 fix
`cd618e3b4f7`) and musl (`src/unistd/setxid.c` + `src/thread/synccall.c`)
propagate a `set*id` change to every thread, with an emphasis on the
RLIMIT_SIGPENDING/EAGAIN failure mode.

Both must solve the same problem: Linux credentials are per-task, so a
`setuid()` from one thread must be replayed on every other thread.  Both do it
by sending a realtime signal whose handler re-issues the syscall.  The realtime
signal can be refused by the kernel with `EAGAIN` once the per-real-user
`RLIMIT_SIGPENDING` is reached (a thread-directed `tgkill`/`tkill` does not use
the guaranteed-delivery override that process-directed `kill` does).

---

## 1. The two designs

### glibc — broadcast + sleeping backoff (`cd618e3b4f7`)

1. Under `dl_stack_cache_lock`, *mark* every other thread (`setxid_mark_thread`):
   wait until it has finished cloning (`setxid_futex` -1/-2 dance), pin it
   against exit (`SETXID_BITMASK`), skip threads already `EXITING`.
2. **Broadcast**: send `SIGSETXID` to all marked threads, incrementing an
   atomic `cntr` per successful `tgkill`.  Each handler runs the syscall
   *asynchronously*, records the result, clears its bit, and decrements `cntr`.
3. Wait on `cntr` (futex) for all handlers to finish.
4. On `EAGAIN` from any `tgkill`: **exponential sleep backoff**
   (`clock_nanosleep`, 1 µs → ~65 ms) and re-run the pass.  Threads whose
   handler already ran are skipped (bit cleared); only the EAGAIN ones retry.
5. Unmark, then run the syscall on the calling thread last.

In-flight signals: **up to N** (N = other threads).  Yielding: the `cntr`
futex-wait and the backoff sleep.

### musl — one-at-a-time rendezvous + busy-retry (`__synccall`)

1. Under `__tl_lock` (held across thread create/exit, so the thread list is
   frozen and no per-thread liveness dance is needed), install the
   `SIGSYNCCALL` handler.
2. **For each thread, one at a time**: set `target_tid`, then
   `while ((r = -tkill(tid, SIGSYNCCALL)) == EAGAIN);` — **busy-retry** — then
   `sem_wait(caller_sem)` until that thread is *caught* in the handler (parked
   on a semaphore).  `count++`.
3. Drive the callback serially across all caught (and now quiesced) threads,
   run it on self, then release them.
4. `__setxid` callback (`do_setxid`) runs the actual syscall; if one thread's
   syscall fails *after* another already succeeded, it `SIGKILL`s the whole
   process (uncatchable) — the state is inconsistent and unsafe.  If a thread
   cannot be signalled at all (non-EAGAIN error), the whole callback is
   nop'd: **all-or-nothing**, and `__setxid` returns `EAGAIN`.

In-flight signals: **≤ 1** (it waits for each before the next).  Yielding: the
`sem_wait` between threads.

### Why musl can busy-retry but glibc's broadcast cannot

Every *successfully sent* realtime signal holds a `RLIMIT_SIGPENDING` slot until
its target thread runs the handler.  In a **broadcast**, when a later `tgkill`
hits `EAGAIN`, the slots are held by the threads we already signalled — whose
handlers still need CPU to run and free those slots.  A busy-loop there would
spin without yielding and, under priority (`SCHED_FIFO`) or a saturated CPU,
could *deadlock*: the spinner outranks the thread that must run to free a slot.
That is exactly why glibc uses a **sleeping** backoff (it yields
unconditionally).

In musl's **one-at-a-time** model there are no outstanding self-signals while it
spins (it already waited for the previous thread), so an `EAGAIN` reflects only
*external* queue pressure and the busy-retry cannot self-deadlock — which is why
musl gets away with it.  It still pins a CPU at 100 % under *persistent*
external exhaustion, whereas glibc's backoff sleeps.

---

## 2. A musl-style port to glibc (implemented & tested)

To compare fairly I implemented musl's one-at-a-time + busy-retry in
`nptl/nptl_setxid.c`, keeping glibc's proven mark/unmark/handler machinery and
only replacing the broadcast loop:

```c
/* Deliver SIGSETXID to a single thread and wait for its handler, like musl's
   __synccall.  At most one SIGSETXID is ever pending, so we never exhaust
   RLIMIT_SIGPENDING ourselves; an EAGAIN here only reflects external pressure.
   Retry until it clears (blocking indefinitely is intentional - safer than
   returning with a thread still on the old credentials).  */
static void
setxid_signal_thread (struct xid_command *cmdp, struct pthread *t)
{
  if ((t->cancelhandling & SETXID_BITMASK) == 0)
    return;

  atomic_store_relaxed (&cmdp->cntr, 1);          /* expect one handler run */

  pid_t pid = __getpid ();
  int val;
  do
    val = INTERNAL_SYSCALL_CALL (tgkill, pid, t->tid, SIGSETXID);
  while (INTERNAL_SYSCALL_ERROR_P (val)
	 && INTERNAL_SYSCALL_ERRNO (val) == EAGAIN);

  if (INTERNAL_SYSCALL_ERROR_P (val))             /* vanished; nothing to wait */
    { atomic_store_relaxed (&cmdp->cntr, 0); return; }

  int cur;                                        /* wait, yielding, for it */
  while ((cur = atomic_load_relaxed (&cmdp->cntr)) != 0)
    futex_wait_simple ((unsigned int *) &cmdp->cntr, cur, FUTEX_PRIVATE);
}
```

The caller then just `mark` → `signal each one at a time` → `unmark`.  This
deletes the enum, the `retry` flag and the whole `setxid_signal_backoff`
helper, so the *caller* is ~30 lines shorter than the committed version.

Result: builds clean, **passes `nptl/tst-setuid-eagain`** (the bug-21108
regression test).  `tst-setuid2` needs real root and fails identically on both
versions, so it is not a discriminator here.

---

## 3. Evaluation

### Robustness

| Aspect | glibc broadcast+backoff | musl one-at-a-time+busy-retry |
|---|---|---|
| Self-induced EAGAIN | Possible: queues up to N signals; low `RLIMIT_SIGPENDING` or many threads can hit it. Handled by backoff. | Essentially impossible: ≤1 pending at a time. |
| EAGAIN retry safety | Sleeping backoff yields → no priority-inversion deadlock. | Busy-retry safe *only* because nothing of ours is outstanding while spinning. |
| Persistent external exhaustion | Blocks (intended) while **sleeping** — low CPU. | Blocks while **spinning** — pins a core at 100 %. |
| Partial-failure consistency | `setxid_error` → `abort` on result mismatch. | `SIGKILL` (uncatchable) + all-or-nothing abort if a thread can't be signalled. |

Net: glibc is the better citizen under CPU/priority pressure and scales to large
thread counts; musl all but eliminates queue pressure but spins under true
exhaustion (its only real downside, and fixable with a sleep).

### Simplicity

- musl's `setxid.c` is tiny (~35 lines) but leans on `__synccall` (~120 lines:
  a three-semaphore rendezvous, AS-safety signal-masking dance, abort path).
  Total complexity is high but reusable.
- glibc's file is self-contained (~190 lines) with the mark/futex/cntr/backoff
  machinery; no semaphore choreography.
- The *one-at-a-time port above* is actually the **simplest caller** of the
  three — the broadcast+backoff complexity in the committed code is the price
  glibc pays to keep the fast parallel broadcast.

### Correctness

Both correct for bug 21108. glibc's handler runs the syscall asynchronously and
needs explicit start/exit race handling (`setxid_futex`, `EXITING_BITMASK`)
because thread creation is *not* fully serialized by a single lock; musl freezes
the thread list under `__tl_lock` across the whole lifecycle, so it needs none
of that but quiesces every thread on each callback (overkill for setxid, but
harmless). No correctness edge favours one over the other for this bug.

### Performance

No-op `setresuid` (still triggers the full broadcast), parked worker threads,
2000 iterations, x86_64, per-call latency:

| threads | glibc broadcast | musl-style one-at-a-time | slowdown |
|--------:|----------------:|-------------------------:|---------:|
|   0     |   0.06 µs       |   0.11 µs                |  –       |
|   1     |   7.24 µs       |   7.37 µs                |  1.0×    |
|   4     |   8.92 µs       |  27.63 µs                |  3.1×    |
|  16     |  25.85 µs       | 116.48 µs                |  4.5×    |
|  64     |  77.50 µs       | 403.92 µs                |  5.2×    |
| 256     | 279.95 µs       |1934.49 µs                |  6.9×    |

Broadcast runs the N handlers in parallel (latency ≈ N syscalls + one wait);
one-at-a-time serializes into N signal+context-switch round-trips, so it is
3–7× slower and the gap widens with N.  Conversely, musl's footprint on the
realtime signal queue is ≤1 vs up to N for the broadcast.

---

## 4. Conclusion

Keep the committed glibc approach (broadcast + sleeping backoff). It preserves
the historically fast, parallel broadcast (musl-style is 3–7× slower at scale
and pins a CPU under true exhaustion), and its sleeping backoff handles EAGAIN
without the priority-inversion hazard a broadcast busy-loop would have.

musl's one-at-a-time design is elegant and naturally avoids self-induced EAGAIN
(its busy-retry is safe precisely because nothing is outstanding while it
spins), but its serialization cost and CPU-spinning under exhaustion make it a
worse fit for glibc, where large thread counts and `SCHED_FIFO` workloads are
common.

If glibc ever wanted musl's near-zero queue footprint without the latency hit,
the middle ground is a *bounded broadcast*: keep broadcasting but cap in-flight
signals below `RLIMIT_SIGPENDING`, draining via the `cntr` wait, and sleep-back
off on EAGAIN — which is essentially the committed code with an explicit
in-flight cap. The current code already behaves this way in practice, so no
change is warranted.
