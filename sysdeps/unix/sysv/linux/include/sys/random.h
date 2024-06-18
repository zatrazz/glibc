#ifndef _LINUX_SYS_RANDOM
#define _LINUX_SYS_RANDOM

# ifndef _ISOMAC
# include <pthreadP.h>

extern void __getrandom_fork_subprocess (void) attribute_hidden;
extern void __getrandom_vdso_release (struct pthread *curp) attribute_hidden;
extern void __getrandom_reset_state (struct pthread *curp) attribute_hidden;
# endif
#endif
