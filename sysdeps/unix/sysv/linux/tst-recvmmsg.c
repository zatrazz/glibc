/* Check recvmmsg timestamp support.
   Copyright (C) 2021 Free Software Foundation, Inc.
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

#include <array_length.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <support/check.h>
#include <support/support.h>
#include <support/xunistd.h>
#include <support/xsocket.h>
#include <sys/mman.h>

/* Some extra space added for ancillary data, it might be used to convert
   32-bit timestamp to 64-bit for _TIME_BITS=64 on older kernels.  */
static const int slack[] = { 0, 4, 8, 16, 32, 64 };

static size_t pagesize;

static int
do_test_send (const struct sockaddr_in *addr, int nmsgs)
{
  int s = xsocket (AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  xconnect (s, (const struct sockaddr *) addr, sizeof (*addr));

  for (int i = 0; i < nmsgs; i++)
    xsendto (s, &i, sizeof (i), 0, (const struct sockaddr *) addr,
	     sizeof (*addr));

  return 0;
}

#define NMSGS 5

static void
do_recvmmsg (int s, bool support_64_timestamp, void *cmsg[], size_t slack,
	     size_t tsize)
{
  /* A timestamp is expected if 32-bit timestamp are used (support in every
     configuration) or if underlying kernel support 64-bit timestamps.
     Otherwise recvmsg will need extra space do add the 64-bit timestamp.  */
  bool exp_timestamp;
  if (sizeof (time_t) == 4 || support_64_timestamp)
    exp_timestamp = true;
  else
    exp_timestamp = slack >= CMSG_SPACE (tsize);

  int msg[5];
  struct iovec iov[NMSGS];
  for (int i = 0; i < NMSGS; i++)
    {
      iov[i].iov_base = &msg[i];
      iov[i].iov_len = sizeof (msg[i]);
    }
  struct mmsghdr msghdr[NMSGS];
  size_t msgs = CMSG_SPACE (tsize + slack);
  for (int i = 0; i < NMSGS; i++)
    {
      msghdr[i].msg_hdr.msg_name = NULL;
      msghdr[i].msg_hdr.msg_namelen = 0;
      msghdr[i].msg_hdr.msg_iov = &iov[i];
      msghdr[i].msg_hdr.msg_iovlen = 1;
      msghdr[i].msg_hdr.msg_controllen = msgs,
      msghdr[i].msg_hdr.msg_control = cmsg[i] != NULL
				      ? cmsg[i] + pagesize - msgs : NULL;
    }
  
  TEST_COMPARE (recvmmsg (s, msghdr, NMSGS, 0, NULL), NMSGS);
  for (int i = 0; i < NMSGS; i++)
    {
      if (msghdr[i].msg_len == 0)
	continue;

      TEST_COMPARE (msg[i], i);

      int timestamp = false;
      for (struct cmsghdr *cmsg = CMSG_FIRSTHDR (&msghdr[i].msg_hdr);
	   cmsg != NULL;
	   cmsg = CMSG_NXTHDR (&msghdr[i].msg_hdr, cmsg))
	{
	  if (cmsg->cmsg_level != SOL_SOCKET)
	    continue;
	  if (cmsg->cmsg_type == SCM_TIMESTAMP
	      && cmsg->cmsg_len == CMSG_LEN (sizeof (struct timeval)))
	    {
	      struct timeval tv;
	      memcpy (&tv, CMSG_DATA (cmsg), sizeof (tv));
	      printf ("SCM_TIMESTAMP:   {%jd, %jd}\n", (intmax_t)tv.tv_sec,
		      (intmax_t)tv.tv_usec);
	      timestamp = true;
	    }
	  else if (cmsg->cmsg_type == SCM_TIMESTAMPNS
		   && cmsg->cmsg_len == CMSG_LEN (sizeof (struct timespec)))
	    {
	      struct timespec ts;
	      memcpy (&ts, CMSG_DATA (cmsg), sizeof (ts));
	      printf ("SCM_TIMESTAMPNS: {%jd, %jd}\n", (intmax_t)ts.tv_sec,
		      (intmax_t)ts.tv_nsec);
	      timestamp = true;
	    }
	}

      /* If there is not timestamp in the ancilliary data, recvmsg should set
	 the flag inidcating it.  */
      if (exp_timestamp && !timestamp)
	TEST_VERIFY (msghdr[i].msg_hdr.msg_flags & MSG_TRUNC);
      else
	TEST_COMPARE (exp_timestamp, timestamp);
    }
}

static int
do_test (void)
{
  int srv = xsocket (AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in srv_addr =
    {
      .sin_family = AF_INET,
      .sin_addr = {.s_addr = htonl (INADDR_LOOPBACK) },
    };
  xbind (srv, (struct sockaddr *) &srv_addr, sizeof (srv_addr));
  {
    socklen_t sa_len = sizeof (srv_addr);
    xgetsockname (srv, (struct sockaddr *) &srv_addr, &sa_len);
    TEST_VERIFY (sa_len == sizeof (srv_addr));
  }

  TEST_COMPARE (recvmmsg (-1, NULL, 0, 0, NULL), -1);
  TEST_COMPARE (errno, EBADF);

  /* Setup the ancillary data buffer with an extra page with PROT_NONE to
     check the possible timestamp conversion on some systems.  */
  pagesize = sysconf (_SC_PAGESIZE);
  if (pagesize == -1)
    FAIL_EXIT1 ("sysconf (_SC_PAGESIZE): %m\n");
  void *msgbuf[NMSGS];
  for (int i = 0; i < NMSGS; i++)
    {
      msgbuf[i] = xmmap (0, 2 * pagesize, PROT_NONE,
			 MAP_ANONYMOUS | MAP_PRIVATE, -1);
      xmprotect (msgbuf[i], pagesize, PROT_READ | PROT_WRITE);
    }

  /* If underlying kernel does not support   */
  bool support_64_timestamp = support_socket_time64_timestamp (srv);

  /* Enable the timestamp using struct timeval precision.  */
  xsetsockopt (srv, SOL_SOCKET, SO_TIMESTAMP, &(int){1}, sizeof (int));
  for (int s = 0; s < array_length (slack); s++)
    {
      do_test_send (&srv_addr, NMSGS);
      do_recvmmsg (srv, support_64_timestamp, msgbuf, slack[s],
		   sizeof (struct timeval));
    }

  /* Now enable timestamp using a higher precision, it overwrites the previous
     precision.  */
  xsetsockopt (srv, SOL_SOCKET, SO_TIMESTAMPNS, &(int){1}, sizeof (int));
  for (int s = 0; s < array_length (slack); s++)
    {
      do_test_send (&srv_addr, NMSGS);
      do_recvmmsg (srv, support_64_timestamp, msgbuf, slack[s],
		   sizeof (struct timespec));
    }

  return 0;
}

#include <support/test-driver.c>
