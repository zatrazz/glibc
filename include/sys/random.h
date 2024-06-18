#ifndef _SYS_RANDOM_H
#include <stdlib/sys/random.h>

# ifndef _ISOMAC

# include <stdbool.h>

extern ssize_t __getrandom (void *__buffer, size_t __length,
                            unsigned int __flags) __wur;
libc_hidden_proto (__getrandom)

extern void __getrandom_fork_subprocess (bool reset_states) attribute_hidden;

# endif /* !_ISOMAC */
#endif
