#ifndef _SPAWN_H
#include <posix/spawn.h>

# ifndef _ISOMAC
#  include <sys/wait.h>

__typeof (posix_spawn) __posix_spawn;
libc_hidden_proto (__posix_spawn)

__typeof (posix_spawn_file_actions_addclose)
  __posix_spawn_file_actions_addclose attribute_hidden;

__typeof (posix_spawn_file_actions_adddup2)
  __posix_spawn_file_actions_adddup2 attribute_hidden;

__typeof (posix_spawn_file_actions_addopen)
  __posix_spawn_file_actions_addopen attribute_hidden;

__typeof (posix_spawn_file_actions_destroy)
  __posix_spawn_file_actions_destroy attribute_hidden;

__typeof (posix_spawn_file_actions_init) __posix_spawn_file_actions_init
  attribute_hidden;

__typeof (posix_spawnattr_init) __posix_spawnattr_init
  attribute_hidden;

__typeof (posix_spawnattr_destroy) __posix_spawnattr_destroy
  attribute_hidden;

__typeof (posix_spawnattr_setflags) __posix_spawnattr_setflags
  attribute_hidden;

__typeof (posix_spawnattr_setsigdefault) __posix_spawnattr_setsigdefault
  attribute_hidden;

__typeof (posix_spawnattr_setsigmask) __posix_spawnattr_setsigmask
  attribute_hidden;

typedef int process_create_id_t;
_Static_assert (sizeof (process_create_id_t) == sizeof (pid_t),
		"process_create_id_t must have same size as pid_t");

/* Create a new process using pidfd_spawn or posix_spawn as a fallback, and
   return an identifier that should be only be used with __spawn_process_wait.
   The identifier is not changed if the process creation fails and the
   function returns the same error code as {pidfd,posix}_spawn.  */
int __spawn_process_create (process_create_id_t *,
			    const char *__restric__,
			    const posix_spawn_file_actions_t *__restrict,
			    const posix_spawnattr_t *__restrict,
			    char *const [__restrict_arr],
			    char *const [__restrict_arr])
     attribute_hidden;

/* Wait for a process created with process_create_id_t, and return the status
   code as for waitpid in second argument.  The third argument is the options
   to be used, for instance WNOHANG.

   It returns either the process id returned by __spawn_process_create for
   the case of pidfd, or the pid_t if the fallback is used.  This semantic
   allows to use this in place of waitpid calls.  */
process_create_id_t __spawn_process_wait (process_create_id_t, int *, int)
     attribute_hidden;

/* Send a signal to the created process using either pidfd_send_signal or
   kill.  */
int __spawn_process_kill (process_create_id_t, int)
     attribute_hidden;

# endif /* !_ISOMAC  */
#endif /* spawn.h  */
