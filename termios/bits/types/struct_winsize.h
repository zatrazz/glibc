/* Definition of struct winsize.
   Copyright (C) 2026 Free Software Foundation, Inc.
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
   <https://www.gnu.org/licenses/>.  */

#ifndef __winsize_defined
#define __winsize_defined 1

/* Type of ARG for TIOCGWINSZ and TIOCSWINSZ requests, and for the
   tcgetwinsize and tcsetwinsize functions.  */
struct winsize
{
  unsigned short int ws_row;	/* Rows, in characters.  */
  unsigned short int ws_col;	/* Columns, in characters.  */

  /* These are not used by the POSIX.1-2024 interfaces.  */
  unsigned short int ws_xpixel;	/* Horizontal pixels.  */
  unsigned short int ws_ypixel;	/* Vertical pixels.  */
};

#endif /* bits/types/struct_winsize.h */
