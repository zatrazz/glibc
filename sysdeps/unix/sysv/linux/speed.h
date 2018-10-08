#ifdef __SPEED_H
#define __SPEED_H

/* Macros to extract the symbolic speed constants from struct termios */
static inline speed_t
c_ospeed (tcflag_t c_cflag)
{
  return c_cflag & CBAUD;
}
static inline speed_t
c_ispeed (tcflag_t c_cflag)
{
  return (c_cflag >> IBSHIFT) & CBAUD;
}
  
/* Internal conversion functions for termios */
baud_t __speed_t_to_baud (speed_t speed);
speed_t __baud_to_speed_t (baud_t speed);

#endif /* speed.h */
