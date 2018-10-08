/* Copyright (C) 1997-2018 Free Software Foundation, Inc.
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

#ifndef _TERMIOS_H
# error "Never include <bits/termios_common.h> directly; use <termios.h> instead."
#endif

/* Definitions comomn to all Linux architectures */

typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	__baud_t;
typedef unsigned int	tcflag_t;

# define _HAVE_STRUCT_TERMIOS_C_ISPEED 1
# define _HAVE_STRUCT_TERMIOS_C_OSPEED 1

/* Same as the -speed functions, but returns an integer baud rate */
#ifdef __USE_MISC
typedef __baud_t baud_t;
extern baud_t cfgetobaud (const struct termios *__termios_p) __THROW;
extern baud_t cfgetibaud (const struct termios *__termios_p) __THROW;
extern int cfsetobaud (struct termios *__termios_p, baud_t __speed) __THROW;
extern int cfsetibaud (struct termios *__termios_p, baud_t __speed) __THROW;
#endif /* __USE_MISC */
