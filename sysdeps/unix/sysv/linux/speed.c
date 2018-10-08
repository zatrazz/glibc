/* `struct termios' speed frobnication functions.  Linux version.
   Copyright (C) 1991-2018 Free Software Foundation, Inc.
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

#include <stddef.h>
#include <errno.h>
#include <termios.h>
#include <speed.h>

/*
 * This is the complete set of legacy B constants defined in
 * various Linux architectures.  This set should not change.
 */
baud_t
__baud_to_speed_t (speed_t speed)
{
  switch (speed) {
#ifdef B0
  case 0:
    return B0;
#endif /* B0 */
#ifdef B50
  case 50:
    return B50;
#endif /* B50 */
#ifdef B75
  case 75:
    return B75;
#endif /* B75 */
#ifdef B110
  case 110:
    return B110;
#endif /* B110 */
#ifdef B134
  case 134:
    return B134;
#endif /* B134 */
#ifdef B150
  case 150:
    return B150;
#endif /* B150 */
#ifdef B200
  case 200:
    return B200;
#endif /* B200 */
#ifdef B300
  case 300:
    return B300;
#endif /* B300 */
#ifdef B600
  case 600:
    return B600;
#endif /* B600 */
#ifdef B1200
  case 1200:
    return B1200;
#endif /* B1200 */
#ifdef B1800
  case 1800:
    return B1800;
#endif /* B1800 */
#ifdef B2400
  case 2400:
    return B2400;
#endif /* B2400 */
#ifdef B4800
  case 4800:
    return B4800;
#endif /* B4800 */
#ifdef B9600
  case 9600:
    return B9600;
#endif /* B9600 */
#ifdef B19200
  case 19200:
    return B19200;
#endif /* B19200 */
#ifdef B38400
  case 38400:
    return B38400;
#endif /* B38400 */
#ifdef B57600
  case 57600:
    return B57600;
#endif /* B57600 */
#ifdef B76800
  case 76800:
    return B76800;
#endif /* B76800 */
#ifdef B115200
  case 115200:
    return B115200;
#endif /* B115200 */
#ifdef B153600
  case 153600:
    return B153600;
#endif /* B153600 */
#ifdef B230400
  case 230400:
    return B230400;
#endif /* B230400 */
#ifdef B307200
  case 307200:
    return B307200;
#endif /* B307200 */
#ifdef B460800
  case 460800:
    return B460800;
#endif /* B460800 */
#ifdef B500000
  case 500000:
    return B500000;
#endif /* B500000 */
#ifdef B576000
  case 576000:
    return B576000;
#endif /* B576000 */
#ifdef B614400
  case 614400:
    return B614400;
#endif /* B614400 */
#ifdef B921600
  case 921600:
    return B921600;
#endif /* B921600 */
#ifdef B1000000
  case 1000000:
    return B1000000;
#endif /* B1000000 */
#ifdef B1152000
  case 1152000:
    return B1152000;
#endif /* B1152000 */
#ifdef B1500000
  case 1500000:
    return B1500000;
#endif /* B1500000 */
#ifdef B2000000
  case 2000000:
    return B2000000;
#endif /* B2000000 */
#ifdef B2500000
  case 2500000:
    return B2500000;
#endif /* B2500000 */
#ifdef B3000000
  case 3000000:
    return B3000000;
#endif /* B3000000 */
#ifdef B3500000
  case 3500000:
    return B3500000;
#endif /* B3500000 */
#ifdef B4000000
  case 4000000:
    return B4000000;
#endif /* B4000000 */
  default:
    return BOTHER;		/* Not a legacy speed */
  }
}
libc_hidden_def (__baud_to_speed_t)

speed_t
__speed_t_to_baud (baud_t speed)
{
  switch (speed)
    {
#ifdef B0
    case B0:
      return 0;
#endif /* B0 */
#ifdef B50
    case B50:
      return 50;
#endif /* B50 */
#ifdef B75
    case B75:
      return 75;
#endif /* B75 */
#ifdef B110
    case B110:
      return 110;
#endif /* B110 */
#ifdef B134
    case B134:
      return 134;
#endif /* B134 */
#ifdef B150
    case B150:
      return 150;
#endif /* B150 */
#ifdef B200
    case B200:
      return 200;
#endif /* B200 */
#ifdef B300
    case B300:
      return 300;
#endif /* B300 */
#ifdef B600
    case B600:
      return 600;
#endif /* B600 */
#ifdef B1200
    case B1200:
      return 1200;
#endif /* B1200 */
#ifdef B1800
    case B1800:
      return 1800;
#endif /* B1800 */
#ifdef B2400
    case B2400:
      return 2400;
#endif /* B2400 */
#ifdef B4800
    case B4800:
      return 4800;
#endif /* B4800 */
#ifdef B9600
    case B9600:
      return 9600;
#endif /* B9600 */
#ifdef B19200
    case B19200:
      return 19200;
#endif /* B19200 */
#ifdef B38400
    case B38400:
      return 38400;
#endif /* B38400 */
#ifdef B57600
    case B57600:
      return 57600;
#endif /* B57600 */
#ifdef B76800
    case B76800:
      return 76800;
#endif /* B76800 */
#ifdef B115200
    case B115200:
      return 115200;
#endif /* B115200 */
#ifdef B153600
    case B153600:
      return 153600;
#endif /* B153600 */
#ifdef B230400
    case B230400:
      return 230400;
#endif /* B230400 */
#ifdef B307200
    case B307200:
      return 307200;
#endif /* B307200 */
#ifdef B460800
    case B460800:
      return 460800;
#endif /* B460800 */
#ifdef B500000
    case B500000:
      return 500000;
#endif /* B500000 */
#ifdef B576000
    case B576000:
      return 576000;
#endif /* B576000 */
#ifdef B614400
    case B614400:
      return 614400;
#endif /* B614400 */
#ifdef B921600
    case B921600:
      return 921600;
#endif /* B921600 */
#ifdef B1000000
    case B1000000:
      return 1000000;
#endif /* B1000000 */
#ifdef B1152000
    case B1152000:
      return 1152000;
#endif /* B1152000 */
#ifdef B1500000
    case B1500000:
      return 1500000;
#endif /* B1500000 */
#ifdef B2000000
    case B2000000:
      return 2000000;
#endif /* B2000000 */
#ifdef B2500000
    case B2500000:
      return 2500000;
#endif /* B2500000 */
#ifdef B3000000
    case B3000000:
      return 3000000;
#endif /* B3000000 */
#ifdef B3500000
    case B3500000:
      return 3500000;
#endif /* B3500000 */
#ifdef B4000000
    case B4000000:
      return 4000000;
#endif /* B4000000 */
    default:
      return speed;		/* Not a legacy speed, treat as baud */
    }
}
libc_hidden_def (__speed_t_to_baud)

/* Return the output baud rate stored in *TERMIOS_P as an integer. */
baud_t
cfgetobaud (const struct termios *termios_p)
{
  speed_t ospeed = c_ospeed (termios_p->c_cflag);

  if (ospeed == BOTHER)
    return termios_p->c_ospeed;
  else
    return __speed_t_to_baud (ospeed);
}
libc_hidden_def (cfgetobaud)

/* Return the input baud rate stored in *TERMIOS_P as an integer. */
baud_t
cfgetibaud (const struct termios *termios_p)
{
  speed_t ispeed = c_ispeed (termios_p->c_cflag);

  if (ispeed == BOTHER)
    return termios_p->c_ispeed;
  else
    return __speed_t_to_baud (ispeed);		/* This includes == B0 */
}
libc_hidden_def (cfgetibaud)

/* Set the output baud rate stored in *TERMIOS_P to SPEED as an integer. */
int
cfsetobaud (struct termios *termios_p, baud_t baud)
{
  speed_t speed;

  termios_p->c_ospeed = baud;
  if (c_ispeed (termios_p->c_cflag) == B0)
    termios_p->c_ispeed = baud;
  speed = __baud_to_speed_t (baud);
  termios_p->c_cflag &= ~CBAUD;
  termios_p->c_cflag |= speed;

  return 0;
}
libc_hidden_def (cfgetobaud)

/* Set the output baud rate stored in *TERMIOS_P to SPEED as an integer. */
int
cfsetibaud (struct termios *termios_p, baud_t baud)
{
  speed_t speed;

  termios_p->c_ispeed = (baud != 0) ? baud : termios_p->c_ospeed;
  speed = __baud_to_speed_t (baud);
  termios_p->c_cflag &= ~CIBAUD;
  termios_p->c_cflag |= (tcflag_t)speed << IBSHIFT;

  return 0;
}
libc_hidden_def (cfsetibaud)

/* Set both input and output speed */
int
cfsetbaud (struct termios *termios_p, baud_t baud)
{
  return cfsetobaud(termios_p, baud) | cfsetibaud(termios_p, baud);
}

/* Return the output baud rate stored in *TERMIOS_P as a symbol. */
speed_t
cfgetospeed (const struct termios *termios_p)
{
  speed_t ospeed = c_ospeed (termios_p->c_cflag);

  if (ospeed != BOTHER)
    return ospeed;
  else
    return __baud_to_speed_t (termios_p->c_ospeed);
}
libc_hidden_def (cfgetospeed)

/* Return the input baud rate stored in *TERMIOS_P as a symbol. */
speed_t
cfgetispeed (const struct termios *termios_p)
{
  speed_t ispeed = c_ispeed (termios_p->c_cflag);

  if (ispeed != BOTHER)		/* This includes == B0 */
    return ispeed;
  else
    return __baud_to_speed_t (termios_p->c_ispeed);
}

/* Set the output baud rate stored in *TERMIOS_P to the symbol SPEED
   if it is a valid symbol, otherwise interpret it as baud. */
int
cfsetospeed (struct termios *termios_p, speed_t speed)
{
  baud_t baud;

  if (speed == BOTHER)
    baud = termios_p->c_ospeed;
  else
    baud = __speed_t_to_baud (speed);

  return cfsetobaud (baud);
}
libc_hidden_def (cfsetospeed)

/* Set the input baud rate stored in *TERMIOS_P to the symbol SPEED
   if it is a valid symbol, otherwise interpret it as baud. */
int
cfsetispeed (struct termios *termios_p, speed_t speed)
{
  baud_t baud;

  if (speed == BOTHER)
    baud = termios_p->c_ispeed;
  else
    baud = __speed_t_to_baud (speed);

  return cfsetibaud (baud);
}
libc_hidden_def (cfsetispeed)
