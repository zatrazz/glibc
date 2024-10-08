/* Checking macros for unistd functions.
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

#ifndef _UNISTD_H
# error "Never include <bits/unistd.h> directly; use <unistd.h> instead."
#endif

extern ssize_t __read_chk (int __fd, void *__buf, size_t __nbytes,
			   size_t __buflen)
  __wur __attr_access ((__write_only__, 2, 3));
extern ssize_t __REDIRECT (__read_alias, (int __fd, void *__buf,
					  size_t __nbytes), read)
  __wur __attr_access ((__write_only__, 2, 3));
__fortify_potential_overload __wur ssize_t
read (int __fd, void *const __clang_pass_object_size0 __buf, size_t __nbytes)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS0_LT (__read_warn, __nbytes, __buf,
					"read called with bigger length than "
					"size of the destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize0 (__buf) != (size_t) -1)
    return __read_chk (__fd, __buf, __nbytes, __glibc_objsize0 (__buf));
  return __read_alias (__fd, __buf, __nbytes);
}
__FORTIFY_FUNCTION_END

#if defined __USE_UNIX98 || defined __USE_XOPEN2K8
extern ssize_t __pread_chk (int __fd, void *__buf, size_t __nbytes,
			    __off_t __offset, size_t __bufsize)
  __wur __attr_access ((__write_only__, 2, 3));
extern ssize_t __pread64_chk (int __fd, void *__buf, size_t __nbytes,
			      __off64_t __offset, size_t __bufsize)
  __wur __attr_access ((__write_only__, 2, 3));
extern ssize_t __REDIRECT (__pread_alias,
			   (int __fd, void *__buf, size_t __nbytes,
			    __off_t __offset), pread)
  __wur __attr_access ((__write_only__, 2, 3));
extern ssize_t __REDIRECT (__pread64_alias,
			   (int __fd, void *__buf, size_t __nbytes,
			    __off64_t __offset), pread64)
  __wur __attr_access ((__write_only__, 2, 3));

# ifndef __USE_FILE_OFFSET64
#  define __fo_pread_chk __pread_chk
#  define __fo_pread_alias __pread_alias
#  define __fo_off_t __off_t
# else
#  define __fo_pread_chk __pread64_chk
#  define __fo_pread_alias __pread64_alias
#  define __fo_off_t __off64_t
# endif

__fortify_potential_overload __wur ssize_t
pread (int __fd, void *const __clang_pass_object_size0 __buf, size_t __nbytes,
       __fo_off_t __offset)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS0_LT (__pread_chk_warn, __nbytes, __buf,
					"pread called with bigger length than "
					"size of the destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize0 (__buf) != (size_t) -1)
    return __fo_pread_chk (__fd, __buf, __nbytes, __offset, __glibc_objsize0 (__buf));
  return __fo_pread_alias (__fd, __buf, __nbytes, __offset);
}
__FORTIFY_FUNCTION_END

#undef __fo_pread_chk
#undef __fo_pread_alias
#undef __fo_off_t

# ifdef __USE_LARGEFILE64
__fortify_potential_overload __wur ssize_t
pread64 (int __fd, void *const __clang_pass_object_size0 __buf,
	  size_t __nbytes, __off64_t __offset)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS0_LT (__pread64_warn, __nbytes, __buf,
					"pread64 called with bigger length "
					"than size of the destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize0 (__buf) != (size_t) -1)
    return __pread64_chk (__fd, __buf, __nbytes, __offset, __glibc_objsize0 (__buf));
  return __pread64_alias (__fd, __buf, __nbytes, __offset);
}
__FORTIFY_FUNCTION_END
# endif
#endif

#if defined __USE_XOPEN_EXTENDED || defined __USE_XOPEN2K
extern ssize_t __readlink_chk (const char *__restrict __path,
			       char *__restrict __buf, size_t __len,
			       size_t __buflen)
     __THROW __nonnull ((1, 2)) __wur __attr_access ((__write_only__, 2, 3));
extern ssize_t __REDIRECT_NTH (__readlink_alias,
			       (const char *__restrict __path,
				char *__restrict __buf, size_t __len), readlink)
     __nonnull ((1, 2)) __wur __attr_access ((__write_only__, 2, 3));

__fortify_potential_overload __nonnull ((1, 2)) __wur ssize_t
__NTH (readlink (const char *__restrict __path,
		 char *__restrict const __clang_pass_object_size __buf,
		 size_t __len))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__readlink_warn, __len, __buf,
				       "readlink called with bigger length "
				       "than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__buf) != (size_t) -1)
    return __readlink_chk (__path, __buf, __len, __glibc_objsize (__buf));
  return __readlink_alias (__path, __buf, __len);
}
__FORTIFY_FUNCTION_END
#endif

#ifdef __USE_ATFILE
extern ssize_t __readlinkat_chk (int __fd, const char *__restrict __path,
				 char *__restrict __buf, size_t __len,
				 size_t __buflen)
     __THROW __nonnull ((2, 3)) __wur __attr_access ((__write_only__, 3, 4));
extern ssize_t __REDIRECT_NTH (__readlinkat_alias,
			       (int __fd, const char *__restrict __path,
				char *__restrict __buf, size_t __len),
			       readlinkat)
     __nonnull ((2, 3)) __wur __attr_access ((__write_only__, 3, 4));

__fortify_potential_overload __nonnull ((2, 3)) __wur ssize_t
__NTH (readlinkat (int __fd,
		   const char *__restrict __path,
		   char *__restrict const __clang_pass_object_size __buf,
		   size_t __len))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__readlinkat_warn, __len, __buf,
				       "readlinkat called with bigger length "
				       "than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__buf) != (size_t) -1)
    return __readlinkat_chk (__fd, __path, __buf, __len, __glibc_objsize (__buf));
  return __readlinkat_alias (__fd, __path, __buf, __len);
}
__FORTIFY_FUNCTION_END
#endif

extern char *__getcwd_chk (char *__buf, size_t __size, size_t __buflen)
     __THROW __wur;
extern char *__REDIRECT_NTH (__getcwd_alias,
			     (char *__buf, size_t __size), getcwd) __wur;

__fortify_potential_overload __wur char *
__NTH (getcwd (char *const __clang_pass_object_size __buf, size_t __size))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__getcwd_warn, __size, __buf,
				       "getcwd called with bigger length than "
				       "size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__buf) != (size_t) -1)
    return __getcwd_chk (__buf, __size, __glibc_objsize (__buf));
  return __getcwd_alias (__buf, __size);
}
__FORTIFY_FUNCTION_END

#if defined __USE_MISC || defined __USE_XOPEN_EXTENDED
# define __warn_getwd_use_something_else \
  "please use getcwd instead, as getwd doesn't specify buffer size"

extern char *__getwd_chk (char *__buf, size_t buflen)
     __THROW __nonnull ((1)) __wur __attr_access ((__write_only__, 1, 2));
extern char *__REDIRECT_NTH (__getwd_warn, (char *__buf), getwd)
     __nonnull ((1)) __wur __warnattr (__warn_getwd_use_something_else);

extern char *__REDIRECT (__getwd_alias, (char *__str), getwd) __wur;

__fortify_potential_overload __nonnull ((1)) __attribute_deprecated__ __wur
char *
__NTH (getwd (char *const __clang_pass_object_size __buf))
     __clang_warning_if (__glibc_objsize (__buf) == (size_t) -1,
			 __warn_getwd_use_something_else)
{
  if (__glibc_objsize (__buf) != (size_t) -1)
    return __getwd_chk (__buf, __glibc_objsize (__buf));
  return __getwd_warn (__buf);
}
# undef __warn_getwd_use_something_else
#endif

extern size_t __confstr_chk (int __name, char *__buf, size_t __len,
			     size_t __buflen) __THROW
  __attr_access ((__write_only__, 2, 3));
extern size_t __REDIRECT_NTH (__confstr_alias, (int __name, char *__buf,
						size_t __len), confstr)
   __attr_access ((__write_only__, 2, 3));

__fortify_potential_overload size_t
__NTH (confstr (int __name, char *const __clang_pass_object_size __buf,
		size_t __len))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__confstr_warn, __len, __buf,
				       "confstr called with bigger length than "
				       "size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__buf) != (size_t) -1)
    return __confstr_chk (__name, __buf, __len, __glibc_objsize (__buf));
  return __confstr_alias (__name, __buf, __len);
}
__FORTIFY_FUNCTION_END


extern int __getgroups_chk (int __size, __gid_t __list[], size_t __listlen)
  __THROW __wur __attr_access ((__write_only__, 2, 1));
extern int __REDIRECT_NTH (__getgroups_alias, (int __size, __gid_t __list[]),
			   getgroups) __wur __attr_access ((__write_only__, 2, 1));

__fortify_potential_overload int
__NTH (getgroups (int __size, __gid_t *const __clang_pass_object_size __list))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__getgroups_warn,
				       __size * sizeof (__gid_t), __list,
				       "getgroups called with bigger group "
				       "count than what can fit into "
				       "destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__list) != (size_t) -1)
    return __getgroups_chk (__size, __list, __glibc_objsize (__list));
  return __getgroups_alias (__size, __list);
}
__FORTIFY_FUNCTION_END


extern int __ttyname_r_chk (int __fd, char *__buf, size_t __buflen,
			    size_t __nreal) __THROW __nonnull ((2))
   __attr_access ((__write_only__, 2, 3));
extern int __REDIRECT_NTH (__ttyname_r_alias, (int __fd, char *__buf,
					       size_t __buflen), ttyname_r)
     __nonnull ((2));
extern int __REDIRECT_NTH (__ttyname_r_chk_warn,
			   (int __fd, char *__buf, size_t __buflen,
			    size_t __nreal), __ttyname_r_chk)
     __nonnull ((2)) __warnattr ("ttyname_r called with bigger buflen than "
				 "size of destination buffer");

__fortify_potential_overload int
__NTH (ttyname_r (int __fd, char *const __clang_pass_object_size __buf,
		  size_t __buflen))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__ttyname_r_warn, __buflen, __buf,
				       "ttyname_r called with bigger buflen "
				       "than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__buf) != (size_t) -1)
    return __ttyname_r_chk (__fd, __buf, __buflen, __glibc_objsize (__buf));
   return __ttyname_r_alias (__fd, __buf, __buflen);
 }
__FORTIFY_FUNCTION_END


#ifdef __USE_POSIX199506
extern int __getlogin_r_chk (char *__buf, size_t __buflen, size_t __nreal)
     __nonnull ((1)) __attr_access ((__write_only__, 1, 2));
extern int __REDIRECT (__getlogin_r_alias, (char *__buf, size_t __buflen),
		       getlogin_r) __nonnull ((1));

__fortify_potential_overload int
getlogin_r (char *const __clang_pass_object_size __buf, size_t __buflen)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__getlogin_r_warn, __buflen, __buf,
				       "getlogin_r called with bigger buflen "
				       "than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__buf) != (size_t) -1)
    return __getlogin_r_chk (__buf, __buflen, __glibc_objsize (__buf));
  return __getlogin_r_alias (__buf, __buflen);
}
__FORTIFY_FUNCTION_END
#endif


#if defined __USE_MISC || defined __USE_UNIX98
extern int __gethostname_chk (char *__buf, size_t __buflen, size_t __nreal)
     __THROW __nonnull ((1)) __attr_access ((__write_only__, 1, 2));
extern int __REDIRECT_NTH (__gethostname_alias, (char *__buf, size_t __buflen),
			   gethostname)
  __nonnull ((1)) __attr_access ((__write_only__, 1, 2));

__fortify_potential_overload int
__NTH (gethostname (char *const __clang_pass_object_size __buf,
		    size_t __buflen))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__gethostname_warn, __buflen, __buf,
				       "gethostname called with bigger buflen "
				       "than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__buf) != (size_t) -1)
    return __gethostname_chk (__buf, __buflen, __glibc_objsize (__buf));
  return __gethostname_alias (__buf, __buflen);
}
__FORTIFY_FUNCTION_END
#endif


#if defined __USE_MISC || (defined __USE_XOPEN && !defined __USE_UNIX98)
extern int __getdomainname_chk (char *__buf, size_t __buflen, size_t __nreal)
     __THROW __nonnull ((1)) __wur __attr_access ((__write_only__, 1, 2));
extern int __REDIRECT_NTH (__getdomainname_alias, (char *__buf,
						   size_t __buflen),
			   getdomainname) __nonnull ((1))
  __wur __attr_access ((__write_only__, 1, 2));

__fortify_potential_overload int
__NTH (getdomainname (char *const __clang_pass_object_size __buf,
		      size_t __buflen))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__getdomainname_warn, __buflen, __buf,
				       "getdomainname called with bigger "
				       "buflen than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__buf) != (size_t) -1)
    return __getdomainname_chk (__buf, __buflen, __glibc_objsize (__buf));
  return __getdomainname_alias (__buf, __buflen);
}
__FORTIFY_FUNCTION_END
#endif
