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
 * speed_t encoding and decoding by swapping the B constants
 * with the corresponding integers.
 *
 * This is the complete set of legacy B constants defined in
 * various Linux architectures.  This set should not change.
 */
speed_t
cfencspeed (baud_t speed)
{
  switch (speed)
    {
#if defined(B0) && (B0 != 0)
    case B0:
      return 0;
    case 0:
      return B0;
#endif /* B0 */
#if defined(B50) && (B50 != 50)
    case B50:
      return 50;
    case 50:
      return B50;
#endif /* B50 */
#if defined(B75) && (B75 != 75)
    case B75:
      return 75;
    case 75:
      return B75;
#endif /* B75 */
#if defined(B110) && (B110 != 110)
    case B110:
      return 110;
    case 110:
      return B110;
#endif /* B110 */
#if defined(B134) && (B134 != 134)
    case B134:
      return 134;
    case 134:
      return B134;
#endif /* B134 */
#if defined(B150) && (B150 != 150)
    case B150:
      return 150;
    case 150:
      return B150;
#endif /* B150 */
#if defined(B200) && (B200 != 200)
    case B200:
      return 200;
    case 200:
      return B200;
#endif /* B200 */
#if defined(B300) && (B300 != 300)
    case B300:
      return 300;
    case 300:
      return B300;
#endif /* B300 */
#if defined(B600) && (B600 != 600)
    case B600:
      return 600;
    case 600:
      return B600;
#endif /* B600 */
#if defined(B1200) && (B1200 != 1200)
    case B1200:
      return 1200;
    case 1200:
      return B1200;
#endif /* B1200 */
#if defined(B1800) && (B1800 != 1800)
    case B1800:
      return 1800;
    case 1800:
      return B1800;
#endif /* B1800 */
#if defined(B2400) && (B2400 != 2400)
    case B2400:
      return 2400;
    case 2400:
      return B2400;
#endif /* B2400 */
#if defined(B4800) && (B4800 != 4800)
    case B4800:
      return 4800;
    case 4800:
      return B4800;
#endif /* B4800 */
#if defined(B9600) && (B9600 != 9600)
    case B9600:
      return 9600;
    case 9600:
      return B9600;
#endif /* B9600 */
#if defined(B19200) && (B19200 != 19200)
    case B19200:
      return 19200;
    case 19200:
      return B19200;
#endif /* B19200 */
#if defined(B38400) && (B38400 != 38400)
    case B38400:
      return 38400;
    case 38400:
      return B38400;
#endif /* B38400 */
#if defined(B57600) && (B57600 != 57600)
    case B57600:
      return 57600;
    case 57600:
      return B57600;
#endif /* B57600 */
#if defined(B76800) && (B76800 != 76800)
    case B76800:
      return 76800;
    case 76800:
      return B76800;
#endif /* B76800 */
#if defined(B115200) && (B115200 != 115200)
    case B115200:
      return 115200;
    case 115200:
      return B115200;
#endif /* B115200 */
#if defined(B153600) && (B153600 != 153600)
    case B153600:
      return 153600;
    case 153600:
      return B153600;
#endif /* B153600 */
#if defined(B230400) && (B230400 != 230400)
    case B230400:
      return 230400;
    case 230400:
      return B230400;
#endif /* B230400 */
#if defined(B307200) && (B307200 != 307200)
    case B307200:
      return 307200;
    case 307200:
      return B307200;
#endif /* B307200 */
#if defined(B460800) && (B460800 != 460800)
    case B460800:
      return 460800;
    case 460800:
      return B460800;
#endif /* B460800 */
#if defined(B500000) && (B500000 != 500000)
    case B500000:
      return 500000;
    case 500000:
      return B500000;
#endif /* B500000 */
#if defined(B576000) && (B576000 != 576000)
    case B576000:
      return 576000;
    case 576000:
      return B576000;
#endif /* B576000 */
#if defined(B614400) && (B614400 != 614400)
    case B614400:
      return 614400;
    case 614400:
      return B614400;
#endif /* B614400 */
#if defined(B921600) && (B921600 != 921600)
    case B921600:
      return 921600;
    case 921600:
      return B921600;
#endif /* B921600 */
#if defined(B1000000) && (B1000000 != 1000000)
    case B1000000:
      return 1000000;
    case 1000000:
      return B1000000;
#endif /* B1000000 */
#if defined(B1152000) && (B1152000 != 1152000)
    case B1152000:
      return 1152000;
    case 1152000:
      return B1152000;
#endif /* B1152000 */
#if defined(B1500000) && (B1500000 != 1500000)
    case B1500000:
      return 1500000;
    case 1500000:
      return B1500000;
#endif /* B1500000 */
#if defined(B2000000) && (B2000000 != 2000000)
    case B2000000:
      return 2000000;
    case 2000000:
      return B2000000;
#endif /* B2000000 */
#if defined(B2500000) && (B2500000 != 2500000)
    case B2500000:
      return 2500000;
    case 2500000:
      return B2500000;
#endif /* B2500000 */
#if defined(B3000000) && (B3000000 != 3000000)
    case B3000000:
      return 3000000;
    case 3000000:
      return B3000000;
#endif /* B3000000 */
#if defined(B3500000) && (B3500000 != 3500000)
    case B3500000:
      return 3500000;
    case 3500000:
      return B3500000;
#endif /* B3500000 */
#if defined(B4000000) && (B4000000 != 4000000)
    case B4000000:
      return 4000000;
    case 4000000:
      return B4000000;
#endif /* B4000000 */
    default:
      return speed;
    }
}
libc_hidden_def (cfencspeed)
strong_alias (cfencspeed, cfdecspeed)

/* Return the output baud rate stored in *TERMIOS_P as a symbol. */
speed_t
cfgetospeed (const struct termios *termios_p)
{
  speed_t ospeed = c_ospeed (termios_p->c_cflag);
  return (ospeed == BOTHER) ? cfencspeed (termios_p->c_ospeed) : ospeed;
}
libc_hidden_def (cfgetospeed)

/* Return the input baud rate stored in *TERMIOS_P as a symbol. */
speed_t
cfgetispeed (const struct termios *termios_p)
{
  speed_t ispeed = c_ispeed (termios_p->c_cflag);
  return (ispeed == BOTHER) ? cfencspeed (termios_p->c_ispeed) : ispeed;
}

/* Set the output baud rate stored in *TERMIOS_P to the symbol SPEED */
int
cfsetospeed (struct termios *termios_p, speed_t speed)
{
  termios_p->c_ospeed = cfdecspeed (speed);
  if ( c_ispeed (termios_p->c_cflag) == B0 )
    termios_p->c_ispeed = termios_p->c_ospeed;

  if ( (speed & ~CBAUD) != 0 || speed == BOTHER || speed <= _MAX_BAUD )
    speed = BOTHER;

  termios_p->c_cflag = (termios_p->c_cflag & ~CBAUD) | speed;
  return 0;
}
libc_hidden_def (cfsetospeed)

/* Set the input baud rate stored in *TERMIOS_P to the symbol SPEED
   if it is a valid symbol, otherwise interpret it as baud. */
int
cfsetispeed (struct termios *termios_p, speed_t speed)
{
  termios_p->c_ispeed = cfdecspeed (speed);

  if ( (speed & ~CBAUD) != 0 || speed == BOTHER || speed <= _MAX_BAUD )
    speed = BOTHER;

  termios_p->c_cflag = (termios_p->c_cflag & ~CIBAUD) | (speed << IBSHIFT);
  return 0;
}
libc_hidden_def (cfsetispeed)
