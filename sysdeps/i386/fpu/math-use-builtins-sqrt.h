/* GCC only expands sqrt and sqrtf inline to fsqrt if excess precision is
   allowed (-fexcess-precision=fast, the GNU C default).  Otherwise it
   emits a library call, which must be redirected to the internal alias.  */
#ifdef I386_EXCESS_PRECISION_STANDARD
# define USE_SQRT_BUILTIN 0
# define USE_SQRTF_BUILTIN 0
#else
# define USE_SQRT_BUILTIN 1
# define USE_SQRTF_BUILTIN 1
#endif
#define USE_SQRTL_BUILTIN 0
#define USE_SQRTF128_BUILTIN 0
