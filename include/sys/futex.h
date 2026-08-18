#ifndef _SYS_FUTEX_H
#include_next <sys/futex.h>

# ifndef _ISOMAC

# include <futex-internal.h>
# include <struct___timespec64.h>

static __always_inline _Bool
__futex_check_flags (unsigned int flags)
{
  return (flags & ~(unsigned int) FUTEX_PRIVATE_FLAG) == 0;
}

# if __TIMESIZE != 64
extern int futex_timedwait (uint32_t *futexp, uint32_t expected,
			    clockid_t clockid,
			    const struct __timespec64 *abstime,
			    unsigned int flags)
     __nonnull ((1));
# endif

# endif /* !_ISOMAC */
#endif
