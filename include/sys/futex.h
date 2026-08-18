#ifndef _SYS_FUTEX_H
#include_next <sys/futex.h>

# ifndef _ISOMAC

# include <futex-internal.h>
# include <struct___timespec64.h>

/* Convert the FUTEX_FLAG_* flags to the futex-internal.h private
   argument (FUTEX_PRIVATE or FUTEX_SHARED), or return a negative value
   if FLAGS contains unknown bits.  */
static __always_inline int
__futex_flags_to_private (unsigned int flags)
{
  if ((flags & ~FUTEX_FLAG_SHARED) != 0)
    return -1;
  return (flags & FUTEX_FLAG_SHARED) ? FUTEX_SHARED : FUTEX_PRIVATE;
}

# if __TIMESIZE != 64
extern int futex_timedwait (uint32_t *futexp, uint32_t expected,
			    clockid_t clockid,
			    const struct __timespec64 *abstime,
			    unsigned int flags)
     __nonnull ((1));
extern int futex_waitv (const struct futex_waiter *waiters,
			unsigned int nwaiters, clockid_t clockid,
			const struct __timespec64 *abstime,
			unsigned int *index)
     __nonnull ((1));
# endif

# endif /* !_ISOMAC */
#endif
