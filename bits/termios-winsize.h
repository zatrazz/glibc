#ifndef __winsize_defined
#define __winsize_defined

/* Type of ARG for TIOCGWINSZ and TIOCSWINSZ requests.  */
struct winsize
{
  unsigned short int ws_row;	/* Rows, in characters.  */
  unsigned short int ws_col;	/* Columns, in characters.  */

  /* These are not actually used.  */
  unsigned short int ws_xpixel;	/* Horizontal pixels.  */
  unsigned short int ws_ypixel;	/* Vertical pixels.  */
};

#define	_IOT_winsize	/* Hurd ioctl type field.  */ \
 _IOT (_IOTS (unsigned short int), 4, 0, 0, 0, 0)

#endif
