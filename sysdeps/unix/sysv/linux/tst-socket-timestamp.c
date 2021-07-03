/* Check recvmsg/recvmmsg timestamp support.
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
#include <support/next_to_fault.h>
#include <sys/mman.h>
#include <sys/param.h>

/* Some extra space added for ancillary data, it might be used to convert
   32-bit timestamp to 64-bit for _TIME_BITS=64 on older kernels.  */
static const int slack[] = { 0, 4, 8, 16, 32, 64 };

static bool support_64_timestamp;

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

static void
do_recvmsg (bool use_multi_call, int s, void *cmsg, size_t slack, size_t tsize,
	    int exp_msg)
{
  /* A timestamp is expected if 32-bit timestamp are used (support in every
     configuration) or if underlying kernel support 64-bit timestamps.
     Otherwise recvmsg will need extra space do add the 64-bit timestamp.  */
  bool exp_timestamp;
  if (sizeof (time_t) == 4 || support_64_timestamp)
    exp_timestamp = true;
  else
    exp_timestamp = slack >= CMSG_SPACE (tsize);

  int msg;
  struct iovec iov =
    {
      .iov_base = &msg,
      .iov_len = sizeof (msg)
    };
  size_t msgs = CMSG_SPACE (tsize) + slack;
  struct mmsghdr mmhdr =
    {
      .msg_hdr =
      {
        .msg_name = NULL,
        .msg_namelen = 0,
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_controllen = msgs,
        .msg_control = cmsg != NULL ? cmsg - msgs : NULL,
      },
    };

  int r;
  if (use_multi_call)
    {
      r = recvmmsg (s, &mmhdr, 1, 0, NULL);
      if (r >= 0)
	r = mmhdr.msg_len;
    }
  else
    r = recvmsg (s, &mmhdr.msg_hdr, 0);
  TEST_COMPARE (r, sizeof (int));
  TEST_COMPARE (msg, exp_msg);

  if (cmsg == NULL)
    return;

  int timestamp = false;
  for (struct cmsghdr *cmsg = CMSG_FIRSTHDR (&mmhdr.msg_hdr);
       cmsg != NULL;
       cmsg = CMSG_NXTHDR (&mmhdr.msg_hdr, cmsg))
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
    TEST_VERIFY (mmhdr.msg_hdr.msg_flags & MSG_TRUNC);
  else
    TEST_COMPARE (exp_timestamp, timestamp);
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

  TEST_COMPARE (recvmsg (-1, NULL, 0), -1);
  TEST_COMPARE (errno, EBADF);
  TEST_COMPARE (recvmmsg (-1, NULL, 0, 0, NULL), -1);
  TEST_COMPARE (errno, EBADF);

  /* Setup the ancillary data buffer with an extra page with PROT_NONE to
     check the possible timestamp conversion on some systems.  */
  size_t fault_size = 0;
  for (int i = 0; i < array_length (slack); i++)
    fault_size = fault_size < slack[i] ? slack[i] : fault_size;
  fault_size += MAX (sizeof (struct timeval), sizeof (struct timespec));
  struct support_next_to_fault ntf = support_next_to_fault_allocate (fault_size);

  /* If underlying kernel does not support   */
  support_64_timestamp = support_socket_time64_timestamp (srv);

  /* Enable the timestamp using struct timeval precision.  */
  {
    int r = setsockopt (srv, SOL_SOCKET, SO_TIMESTAMP, &(int){1},
			sizeof (int));
    TEST_VERIFY_EXIT (r != -1);
  }
  /* Check recvmsg.  */
  do_test_send (&srv_addr, array_length (slack));
  for (int s = 0; s < array_length (slack); s++)
    do_recvmsg (false, srv, ntf.buffer + ntf.length, slack[s],
		sizeof (struct timeval), s);
  /* Check recvmmsg.  */
  do_test_send (&srv_addr, array_length (slack));
  for (int s = 0; s < array_length (slack); s++)
    do_recvmsg (true, srv, ntf.buffer + ntf.length, slack[s],
		sizeof (struct timeval), s);

  /* Now enable timestamp using a higher precision, it overwrites the previous
     precision.  */
  {
    int r = setsockopt (srv, SOL_SOCKET, SO_TIMESTAMPNS, &(int){1},
			sizeof (int));
    TEST_VERIFY_EXIT (r != -1);
  }
  /* Check recvmsg.  */
  do_test_send (&srv_addr, array_length (slack));
  for (int s = 0; s < array_length (slack); s++)
    do_recvmsg (false, srv, ntf.buffer + ntf.length, slack[s],
		sizeof (struct timespec), s);
  /* Check recvmmsg.  */
  do_test_send (&srv_addr, array_length (slack));
  for (int s = 0; s < array_length (slack); s++)
    do_recvmsg (true, srv, ntf.buffer + ntf.length, slack[s],
		sizeof (struct timespec), s);

  return 0;
}

#include <support/test-driver.c>
