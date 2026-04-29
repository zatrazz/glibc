#ifndef _SPAWN_EXT
# define _SPAWN_EXT

#include_next <bits/spawn_ext.h>

#  ifndef _ISOMAC
__typeof (pidfd_spawn) __pidfd_spawn;
libc_hidden_proto (__pidfd_spawn);
# endif

#endif
