/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <libc-lock.h>
#include <libio/libioP.h>
#include <math_ldbl_opt.h>
#include <paths.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <syslog.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <stdio_ext.h>

#ifndef HOST_NAME_MAX
# define HOST_NAME_MAX 64
#endif

static int LogType = SOCK_DGRAM;        /* Type of socket connection  */
static int LogFile = -1;                /* fd for log  */
static bool connected;                  /* Have done connect */
static int LogStat;                     /* Status bits, set by openlog()  */
static const char *LogTag;              /* String to tag the entry with  */
static int LogFacility = LOG_USER;      /* Default facility code  */
static int LogMask = 0xff;              /* Mask of priorities to be logged  */
extern char *__progname;                /* Program name, from crt0.  */

/* Define the lock.  */
__libc_lock_define_initialized (static, syslog_lock)
static void openlog_internal (const char *, int, int);
static void closelog_internal (void);

struct cleanup_arg
{
  void *buf;
  struct sigaction *oldaction;
};

static void cancel_handler (void *ptr)
{
  /* Restore the old signal handler.  */
  struct cleanup_arg *clarg = (struct cleanup_arg *) ptr;

  if (clarg != NULL)
    /* Free the memstream buffer,  */
    free (clarg->buf);

  /* Free the lock.  */
  __libc_lock_unlock (syslog_lock);
}


/*
 * syslog, vsyslog --
 *     print message on log file; output is intended for syslogd(8).
 */
void
__syslog (int pri, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  __vsyslog_internal (pri, fmt, ap, 0);
  va_end (ap);
}
ldbl_hidden_def (__syslog, syslog)
ldbl_strong_alias (__syslog, syslog)

void
__vsyslog (int pri, const char *fmt, va_list ap)
{
  __vsyslog_internal (pri, fmt, ap, 0);
}
ldbl_weak_alias (__vsyslog, vsyslog)

void
__syslog_chk (int pri, int flag, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  __vsyslog_internal (pri, fmt, ap, (flag > 0) ? PRINTF_FORTIFY : 0);
  va_end (ap);
}

void
__vsyslog_chk (int pri, int flag, const char *fmt, va_list ap)
{
  __vsyslog_internal (pri, fmt, ap, (flag > 0) ? PRINTF_FORTIFY : 0);
}

/* Defined by RFC5424.  */
#define NILVALUE "-"

struct timebuf_t
{
  char b[sizeof ("YYYY-MM-DDThh:mm:ss.nnnnnnZ")];
};

/* Fill TIMEBUF with a RFC3339 timestamp.  Use UTC time and maximum
   TIME-SECFRAC accurancy allowed (6 digits for microseconds).  */
static void
syslog_rfc3339_timestamp (struct timebuf_t *timebuf)
{
  struct __timespec64 ts = timespec64_now ();
  struct tm now_tm;
  __gmtime64_r (&ts.tv_sec, &now_tm);

  __snprintf (timebuf->b, sizeof (timebuf->b),
              "%04d-%02d-%02dT%02d:%02d:%02d.%06dZ",
              now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday,
              now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec,
              (int32_t) ts.tv_nsec / 1000);
}

void
__vsyslog_internal (int pri, const char *fmt, va_list ap,
                    unsigned int mode_flags)
{
  char *buf = NULL;
  size_t bufsize = 0;
  bool buf_to_free = false;
  int msgoff;
  int saved_errno = errno;

#define INTERNALLOG LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID
  /* Check for invalid bits.  */
  if (pri & ~(LOG_PRIMASK | LOG_FACMASK))
    {
      syslog (INTERNALLOG, "syslog: unknown facility/priority: %x", pri);
      pri &= LOG_PRIMASK | LOG_FACMASK;
    }

  /* Prepare for multiple users.  We have to take care: most
     syscalls we are using are cancellation points.  */
  struct cleanup_arg clarg = { NULL, NULL };
  __libc_cleanup_push (cancel_handler, &clarg);
  __libc_lock_lock (syslog_lock);

  /* Check priority against setlogmask values.  */
  if ((LOG_MASK (LOG_PRI (pri)) & LogMask) == 0)
    goto out;

  /* Set default facility if none specified.  */
  if ((pri & LOG_FACMASK) == 0)
    pri |= LogFacility;

  struct timebuf_t timestamp;
  syslog_rfc3339_timestamp (&timestamp);

  char hostname[HOST_NAME_MAX];
  if (__gethostname (hostname, sizeof (hostname)) == -1)
    strcpy (hostname, NILVALUE);

  pid_t pid = LogStat & LOG_PID ? __getpid () : 0;

#define SYSLOG_HEADER(__pri, __timestamp, __msgoff, __hostname, __pid) \
  "<%d>1 %s %n%s %s %s%.0d %s %s ",                      \
  __pri,                                /* PRI  */       \
  __timestamp.b,                        /* TIMESTAMP  */ \
  __msgoff, __hostname,                 /* HOSTNAME  */  \
  LogTag == NULL ? __progname : LogTag, /* APP-NAME  */  \
  NILVALUE + !!__pid, __pid,            /* PROCID  */    \
  NILVALUE,                             /* MSGID  */     \
  NILVALUE                              /* STRUCT-DATA */

  /* Try to use a static buffer as an optimization.  */
  enum { bufs_size  = 1024 };
  char bufs[bufs_size];
  int l = __snprintf (bufs, sizeof bufs,
                      SYSLOG_HEADER (pri, timestamp, &msgoff, hostname, pid));
  if (l < sizeof (bufs))
    {
      va_list apc;
      va_copy (apc, ap);

      /* Restore errno for %m format.  */
      __set_errno (saved_errno);
      int vl = __vsnprintf_internal (bufs + l, sizeof (bufs) - l, fmt, apc,
                                     mode_flags);
      if (l + vl < sizeof (bufs))
        {
          buf = bufs;
          bufsize = l + vl;
        }

      va_end (apc);
    }

  /* If the required size is larger than buffer size fallbacks to
     open_memstream.  */
  if (buf == NULL)
    {
      FILE *f = __open_memstream (&buf, &bufsize);
      if (f != NULL)
        {
          __fsetlocking (f, FSETLOCKING_BYCALLER);
          fprintf (f, SYSLOG_HEADER (pri, timestamp, &msgoff, hostname, pid));
          /* Restore errno for %m format.  */
          __set_errno (saved_errno);
          __vfprintf_internal (f, fmt, ap, mode_flags);
          fclose (f);

          /* Tell the cancellation handler to free this buffer.  */
          buf_to_free = true;
          clarg.buf = buf;
        }
      else
        {
          bufsize = __snprintf (bufs, sizeof bufs,
                                "out of memory[%d]", __getpid ());
          buf = bufs;
        }
    }

  /* Output to stderr if requested.  */
  if (LogStat & LOG_PERROR)
    __dprintf (STDERR_FILENO, "%s%s", buf + msgoff,
               buf[bufsize - 1] != '\n' ? "\n" : "");

  /* Get connected, output the message to the local logger.  */
  if (!connected)
    openlog_internal (LogTag, LogStat | LOG_NDELAY, 0);

  /* If we have a SOCK_STREAM connection, also send ASCII NUL as
     a record terminator.  */
  if (LogType == SOCK_STREAM)
    ++bufsize;

  if (!connected || __send (LogFile, buf, bufsize, MSG_NOSIGNAL) < 0)
    {
      if (connected)
        {
          /* Try to reopen the syslog connection.  Maybe it went down.  */
          closelog_internal ();
          openlog_internal (LogTag, LogStat | LOG_NDELAY, 0);
        }

      if (!connected || __send (LogFile, buf, bufsize, MSG_NOSIGNAL) < 0)
        {
          closelog_internal ();        /* attempt re-open next time  */
          /*
           * Output the message to the console; don't worry
           * about blocking, if console blocks everything will.
           * Make sure the error reported is the one from the
           * syslogd failure.
           */
          int fd;
          if (LogStat & LOG_CONS &&
              (fd = __open (_PATH_CONSOLE, O_WRONLY | O_NOCTTY | O_CLOEXEC,
                            0)) >= 0)
            {
              __dprintf (fd, "%s\r\n", buf + msgoff);
              __close (fd);
            }
        }
    }

out:
  /* End of critical section.  */
  __libc_cleanup_pop (0);
  __libc_lock_unlock (syslog_lock);

  if (buf_to_free)
    free (buf);
}

/* AF_UNIX address of local logger  */
static const struct sockaddr_un SyslogAddr =
  {
    .sun_family = AF_UNIX,
    .sun_path = _PATH_LOG
  };

static void
openlog_internal (const char *ident, int logstat, int logfac)
{
  if (ident != NULL)
    LogTag = ident;
  LogStat = logstat;
  if (logfac != 0 && (logfac & ~LOG_FACMASK) == 0)
    LogFacility = logfac;

  int retry = 0;
  while (retry < 2)
    {
      if (LogFile == -1)
        {
          if (LogStat & LOG_NDELAY)
            {
              LogFile = __socket (AF_UNIX, LogType | SOCK_CLOEXEC, 0);
              if (LogFile == -1)
                return;
            }
        }
      if (LogFile != -1 && !connected)
        {
          int old_errno = errno;
          if (__connect (LogFile, &SyslogAddr, sizeof (SyslogAddr)) == -1)
            {
              int saved_errno = errno;
              int fd = LogFile;
              LogFile = -1;
              __close (fd);
              __set_errno (old_errno);
              if (saved_errno == EPROTOTYPE)
                {
                  /* retry with the other type:  */
                  LogType = (LogType == SOCK_DGRAM
                            ? SOCK_STREAM : SOCK_DGRAM);
                  ++retry;
                  continue;
                }
            }
          else
            connected = true;
        }
      break;
    }
}

void
openlog (const char *ident, int logstat, int logfac)
{
  /* Protect against multiple users and cancellation.  */
  __libc_cleanup_push (cancel_handler, NULL);
  __libc_lock_lock (syslog_lock);

  openlog_internal (ident, logstat, logfac);

  __libc_cleanup_pop (1);
}

static void
closelog_internal (void)
{
  if (!connected)
    return;

  __close (LogFile);
  LogFile = -1;
  connected = false;
}

void
closelog (void)
{
  /* Protect against multiple users and cancellation.  */
  __libc_cleanup_push (cancel_handler, NULL);
  __libc_lock_lock (syslog_lock);

  closelog_internal ();
  LogTag = NULL;
  LogType = SOCK_DGRAM; /* this is the default */

  /* Free the lock.  */
  __libc_cleanup_pop (1);
}

/* setlogmask -- set the log mask level  */
int
setlogmask (int pmask)
{
  int omask;

  /* Protect against multiple users.  */
  __libc_lock_lock (syslog_lock);

  omask = LogMask;
  if (pmask != 0)
    LogMask = pmask;

  __libc_lock_unlock (syslog_lock);

  return (omask);
}
