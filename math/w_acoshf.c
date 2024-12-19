#include <math-type-macros-float.h>
#undef __USE_WRAPPER_TEMPLATE
#define __USE_WRAPPER_TEMPLATE 1
#undef declare_mgen_alias
#define declare_mgen_alias(a, b)
#include <w_acosh_template.c>
versioned_symbol (libm, __acoshf, acoshf, GLIBC_2_42);
libm_alias_float_other (__acoshf, acoshf)
