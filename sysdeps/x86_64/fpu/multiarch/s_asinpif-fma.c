#define __asinpif __asinpif_fma
#define SECTION __attribute__ ((section (".text.fma")))

#include <sysdeps/ieee754/flt-32/s_asinpif.c>
