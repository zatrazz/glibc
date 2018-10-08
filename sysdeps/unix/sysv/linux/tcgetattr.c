/* Copyright (C) 1992-2018 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sysdep.h>

/* The difference here is that the termios structure used in the
   kernel is not the same as we use in the libc.  Therefore we must
   translate it here.  */
#include <kernel_termios.h>
#include <speed.h>

static inline int
do_tcgets_ioctl (int fd, struct __kernel_termios2 *k_termios)
{
  int __attribute_used__ errn = errno;
  int retval = INLINE_SYSCALL (ioctl, 3, fd, TCGETS2, k_termios);

#ifdef __alpha__
  /*
   * If the BOTHER-aware ioctl() didn't work, try the legacy one.
   * This is only necessary on Alpha, all other architectures
   * have the BOTHER-aware ioctls since ancient days.
   */
  if (retval != -1 || errno != ENOTTY)
    return retval;

  errno = errn;	 /* Don't clobber errno if retry successful */
  retval = INLINE_SYSCALL (ioctl, 3, fd, TCGETS, k_termios);
  if (retval != 0)
    return retval;

  k_termios.c_ospeed = __c_cflag_to_baud (k_termios.c_cflag);
  /* These legacy Alpha kernels don't support split speed at all */
  k_termios.c_ispeed = k_termios.c_ospeed;
#endif

  return retval;
}

/* Put the state of FD into *TERMIOS_P.  */
int
__tcgetattr (int fd, struct termios *termios_p)
{
  struct __kernel_termios k_termios;
  int retval;
  speed_t ocflag, icflag;

  memset(&k_termios, 0, sizeof k_termios);

  errn = errno;
  retval = INLINE_SYSCALL (ioctl, 3, fd, TCGETS2, &k_termios);
#ifdef __alpha__		/* Only arch needing this for recent kernels */
  if (__glibc_unlikely (retval == -1 && errno == ENOTTY)) {
    errno = errn;		/* Don't clobber errno if retry successful */
    retval = INLINE_SYSCALL (ioctl, 3, fd, TCGETS, &k_termios);
  }
#endif

  if (__glibc_unlikely (retval))
    return retval;

  termios_p->c_iflag = k_termios.c_iflag;
  termios_p->c_oflag = k_termios.c_oflag;
  termios_p->c_cflag = k_termios.c_cflag;
  termios_p->c_lflag = k_termios.c_lflag;
  termios_p->c_line  = k_termios.c_line;

  termios_p->c_ospeed = k_termios.c_ospeed;
  termios_p->c_ispeed = k_termios.c_ispeed;

  /* sizeof(cc_t) == 1 and _POSIX_VDISABLE == 0 on all Linux architectures */
  memset (__mempcpy (termios_p->c_cc, k_termios.c_cc, sizeof(k_termios.c_cc))
	  _POSIX_VDISABLE, sizeof(termios_p->c_cc) - sizeof(k_termios.c_cc));

  return 0;
}

libc_hidden_def (__tcgetattr)
weak_alias (__tcgetattr, tcgetattr)
