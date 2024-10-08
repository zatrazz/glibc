/* Checking macros for socket functions.
   Copyright (C) 2005-2023 Free Software Foundation, Inc.
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

#ifndef _SYS_SOCKET_H
# error "Never include <bits/socket2.h> directly; use <sys/socket.h> instead."
#endif

extern ssize_t __recv_chk (int __fd, void *__buf, size_t __n, size_t __buflen,
			   int __flags);
extern ssize_t __REDIRECT (__recv_alias, (int __fd, void *__buf, size_t __n,
					  int __flags), recv);
__fortify_potential_overload ssize_t
recv (int __fd, void *const __clang_pass_object_size0 __buf, size_t __n,
      int __flags)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS0_LT (__recv_warn, __n, __buf,
					"recv called with bigger length than "
					"size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize0 (__buf) != (size_t) -1)
    return __recv_chk (__fd, __buf, __n, __glibc_objsize0 (__buf), __flags);
  return __recv_alias (__fd, __buf, __n, __flags);
}
__FORTIFY_FUNCTION_END

extern ssize_t __recvfrom_chk (int __fd, void *__restrict __buf, size_t __n,
			       size_t __buflen, int __flags,
			       __SOCKADDR_ARG __addr,
			       socklen_t *__restrict __addr_len);
extern ssize_t __REDIRECT (__recvfrom_alias,
			   (int __fd, void *__restrict __buf, size_t __n,
			    int __flags, __SOCKADDR_ARG __addr,
			    socklen_t *__restrict __addr_len), recvfrom);

__fortify_potential_overload ssize_t
recvfrom (int __fd, void *__restrict const __clang_pass_object_size0 __buf,
	  size_t __n, int __flags, __SOCKADDR_ARG __addr,
	  socklen_t *__restrict __addr_len)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS0_LT (__recvfrom_warn, __n, __buf,
					"recvfrom called with bigger length "
					"than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize0 (__buf) != (size_t) -1)
    return __recvfrom_chk (__fd, __buf, __n, __glibc_objsize0 (__buf), __flags,
			   __addr, __addr_len);
  return __recvfrom_alias (__fd, __buf, __n, __flags, __addr, __addr_len);
}
__FORTIFY_FUNCTION_END
