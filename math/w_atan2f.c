#include <math-type-macros-float.h>
#undef __USE_WRAPPER_TEMPLATE
#define __USE_WRAPPER_TEMPLATE 1
#undef declare_mgen_alias
#define declare_mgen_alias(a, b)
#include <w_atan2_template.c>
versioned_symbol (libm, __atan2f, atan2f, GLIBC_2_43);
libm_alias_float_other (__atan2f, atan2f)
