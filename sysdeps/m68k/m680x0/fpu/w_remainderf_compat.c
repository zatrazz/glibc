/* m68k provides an optimized remainderf.  */
#ifdef SHARED
# define NO_COMPAT_NEEDED 1
# include <math/w_remainderf_compat.c>
#else
# include <math-type-macros-float.h>
# include <w_remainder_template.c>
#endif
