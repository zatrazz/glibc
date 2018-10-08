/* Copyright (C) 1993-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sysdep.h>

/* The difference here is that the termios structure used in the
   kernel is not the same as we use in the libc.  Therefore we must
   translate it here.  */
#include <kernel_termios.h>
#include <speed.h>

static inline int
do_tcsets_ioctl (int fd, unsigned long cmd,
		 const struct __kernel_termios2 *k_termios)
{
  int __attribute_used__ errn = errno;
  int retval = INLINE_SYSCALL (ioctl, 3, fd, cmd, &k_termios);

  /*
   * If the BOTHER-aware ioctl() didn't work, try the legacy one.
   * This is only necessary on Alpha, all other architectures
   * have the BOTHER-aware ioctls since ancient days.
   */
#ifdef __alpha__
  if (retval != -1 || errno != ENOTTY)
    return retval;

  /* If this is a legacy Alpha kernel, it supports neither BOTHER nor IBSHIFT */
  speed_t icbaud = c_ispeed (k_termios->c_cflag);
  speed_t ocbaud = c_ospeed (k_termios->c_cflag);

  if (icbaud == BOTHER || ocbaud == BOTHER ||
      (icbaud != B0 && icbaud != ocbaud))
    return INLINE_SYSCALL_ERROR_RETURN_VALUE (EINVAL);

  cmd += TCSETS - TCSETS2;
  errno = errn;		   /* Don't clobber errno if retry succeeds */
  retval = INLINE_SYSCALL (ioctl, 3, fd, cmd, &k_termios);
#endif

  return retval;
}

/* Set the state of FD to *TERMIOS_P.  */
int
__tcsetattr (int fd, int optional_actions, const struct termios *termios_p)
{
  struct __kernel_termios2 k_termios;
  unsigned long int cmd;
  tcflag_t c_cflag;
  int errn, retval;

  /*
   * These relationships are guaranteed on all Linux architectures,
   * present and future.
   */
  cmd = optional_actions - TCSANOW;
  if (cmd > (TCAFLUSH - TCSANOW))
    return INLINE_SYSCALL_ERROR_RETURN_VALUE (EINVAL);
  cmd += TCSETS2;

  k_termios.c_iflag = termios_p->c_iflag;
  k_termios.c_oflag = termios_p->c_oflag;
  k_termios.c_cflag = c_cflag = termios_p->c_cflag;
  k_termios.c_lflag = termios_p->c_lflag;
  k_termios.c_line = termios_p->c_line;
  k_termios.c_ispeed = termios_p->c_ispeed;
  k_termios.c_ospeed = termios_p->c_ospeed;
  memcpy (k_termios.c_cc, termios_p->c_cc, sizeof(k_termios.c_cc));

  /* If the speed is set to BOTHER, *try* to convert it to a legacy flag */
  if (c_ispeed (termios_p->c_cflag) == BOTHER)
    {
      speed_t ispeed = __baud_to_speed_t (termios_p->c_ispeed);
      k_termios.c_cflag &= ~CIBAUD;
      k_termios.c_cflag |= ispeed << IBSHIFT;
    }
  if (c_ospeed (termios_p->c_cflag) == BOTHER)
    {
      speed_t ospeed = __baud_to_speed_t (termios_p->c_ospeed);
      k_termios.c_cflag &= ~CBAUD;
      k_termios.c_cflag |= ospeed;
    }

  return do_tcsets_ioctl(fd, cmd, &k_termios);
}
weak_alias (__tcsetattr, tcsetattr)
libc_hidden_def (tcsetattr)
