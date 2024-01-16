#ifndef __time_t_defined
#define __time_t_defined 1

#include <bits/types.h>

/* Returned by `time'.  */
#if __USE_TIME_BITS64 && __TIMESIZE == 32
typedef __time64_t time_t;
#else
typedef __time_t time_t;
#endif

#endif
