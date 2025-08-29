#if __ARCH__ > 6
# define USE_CLZ_BUILTIN 1
# define USE_CTZ_BUILTIN 1
#else
# define USE_CLZ_BUILTIN 0
# define USE_CTZ_BUILTIN 0
#endif
