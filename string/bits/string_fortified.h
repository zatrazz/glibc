/* Copyright (C) 2004-2023 Free Software Foundation, Inc.
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

#ifndef _BITS_STRING_FORTIFIED_H
#define _BITS_STRING_FORTIFIED_H 1

#ifndef _STRING_H
# error "Never use <bits/string_fortified.h> directly; include <string.h> instead."
#endif

#define __warn_len_too_large \
  "function called with bigger length than the destination buffer"
/* Repeat bodies here to reduce 'note's if we detect a problem.  */
#define __size_too_small(bos, dest, len) \
  (bos (dest) != (size_t) -1 && bos (dest) < len)
#define __warn_if_dest_too_small(dest, len) \
  __clang_warning_if (__size_too_small (__glibc_objsize, dest, len), \
					__warn_len_too_large)
#define __warn_if_dest_too_small0(dest, len) \
  __clang_warning_if (__size_too_small (__glibc_objsize0, dest, len), \
					__warn_len_too_large)

#define __warn_input_str_too_large \
  "destination buffer will always be overflown by source"
#define __warn_if_src_too_large(dest, src) \
  __clang_warning_if (__size_too_small (__glibc_objsize, dest, __builtin_strlen (src) + 1), \
		      __warn_input_str_too_large)

__fortify_potential_overload void *
__NTH (memcpy (void *__restrict const __clang_pass_object_size0 __dest,
	       const void *__restrict __src, size_t __len))
     __warn_if_dest_too_small0 (__dest, __len)
{
  size_t __glibc_objsize_dst = __glibc_objsize0 (__dest);
  if (__glibc_objsize_dst == (size_t) -1 || (__builtin_constant_p (__len)
				   && __glibc_objsize_dst >= __len))
    return __builtin_memcpy (__dest, __src, __len);
  return __builtin___memcpy_chk (__dest, __src, __len, __glibc_objsize_dst);
}

__fortify_potential_overload void *
__NTH (memmove (void *const __clang_pass_object_size0 __dest,
		const void *__src, size_t __len))
     __warn_if_dest_too_small0 (__dest, __len)
{
  size_t __glibc_objsize_dst = __glibc_objsize0 (__dest);
  if (__glibc_objsize_dst == (size_t) -1 || (__builtin_constant_p (__len)
				   && __glibc_objsize_dst >= __len))
    return __builtin_memmove (__dest, __src, __len);
  return __builtin___memmove_chk (__dest, __src, __len, __glibc_objsize_dst);
}

#ifdef __USE_GNU
__fortify_potential_overload void *
__NTH (mempcpy (void *__restrict const __clang_pass_object_size0 __dest,
		const void *__restrict __src, size_t __len))
     __warn_if_dest_too_small0 (__dest, __len)
{
  size_t __glibc_objsize_dst = __glibc_objsize0 (__dest);
  if (__glibc_objsize_dst == (size_t) -1 || (__builtin_constant_p (__len)
				   && __glibc_objsize_dst >= __len))
    return __builtin_mempcpy (__dest, __src, __len);
  return __builtin___mempcpy_chk (__dest, __src, __len, __glibc_objsize_dst);
}
#endif

/* The first two tests here help to catch a somewhat common problem
   where the second and third parameter are transposed.  This is
   especially problematic if the intended fill value is zero.  In this
   case no work is done at all.  We detect these problems by referring
   non-existing functions.  */
#define __warn_memset_zero_len_msg \
  "memset used with constant zero length parameter; this could be due to " \
  "transposed parameters"
__fortify_potential_overload void *
__NTH (memset (void *const __clang_pass_object_size0 __dest, int __ch,
	       size_t __len))
     __warn_if_dest_too_small0 (__dest, __len)
     __clang_warning_if (__len == 0 && __ch != 0, __warn_memset_zero_len_msg)
{
  size_t __glibc_objsize_dst = __glibc_objsize0 (__dest);
  if (__glibc_objsize_dst == (size_t) -1 || (__builtin_constant_p (__len)
				   && __glibc_objsize_dst >= __len))
    return __builtin_memset (__dest, __ch, __len);
  return __builtin___memset_chk (__dest, __ch, __len, __glibc_objsize_dst);
}
#undef __warn_memset_zero_len_msg

#ifdef __USE_MISC
# include <bits/strings_fortified.h>

void __explicit_bzero_chk (void *__dest, size_t __len, size_t __destlen)
  __THROW __nonnull ((1)) __fortified_attr_access (__write_only__, 1, 2);

__fortify_function void
__NTH (explicit_bzero (void *__dest, size_t __len))
{
  __explicit_bzero_chk (__dest, __len, __glibc_objsize0 (__dest));
}
#endif

__fortify_potential_overload char *
__NTH (strcpy (char *__restrict const __clang_pass_object_size __dest,
	       const char *__restrict __src))
     __warn_if_src_too_large (__dest, __src)
{
  return __builtin___strcpy_chk (__dest, __src, __glibc_objsize (__dest));
}

#ifdef __USE_XOPEN2K8
__fortify_potential_overload char *
__NTH (stpcpy (char *__restrict const __clang_pass_object_size __dest,
	       const char *__restrict __src))
     __warn_if_src_too_large (__dest, __src)
{
  return __builtin___stpcpy_chk (__dest, __src, __glibc_objsize (__dest));
}
#endif

__fortify_potential_overload char *
__NTH (strncpy (char *__restrict const __clang_pass_object_size __dest,
		const char *__restrict __src, size_t __len))
/* clang: Don't warn when __builtin_strlen (__src) < __glibc_objsize (__dest),
   but __len > __glibc_objsize (__dest).  The user should fix their code instead.  */
     __warn_if_dest_too_small (__dest, __len)
{
  return __builtin___strncpy_chk (__dest, __src, __len,
				  __glibc_objsize (__dest));
}

extern char *__stpncpy_chk (char *__dest, const char *__src, size_t __n,
			    size_t __destlen) __THROW
  __fortified_attr_access (__write_only__, 1, 3)
  __attr_access ((__read_only__, 2));
extern char *__REDIRECT_NTH (__stpncpy_alias, (char *__dest, const char *__src,
					       size_t __n), stpncpy);

__fortify_potential_overload char *
__NTH (stpncpy (char *const __clang_pass_object_size __dest, const char *__src,
		size_t __n))
     __warn_if_dest_too_small (__dest, __n)
{
  if (__glibc_objsize (__dest) != (size_t) -1)
    return __stpncpy_chk (__dest, __src, __n, __glibc_objsize (__dest));
  return __stpncpy_alias (__dest, __src, __n);
}

__fortify_potential_overload char *
__NTH (strcat (char *__restrict const __clang_pass_object_size __dest,
	       const char *__restrict __src))
     __warn_if_src_too_large (__dest, __src)
{
  return __builtin___strcat_chk (__dest, __src, __glibc_objsize (__dest));
}

__fortify_potential_overload char *
__NTH (strncat (char *__restrict const __clang_pass_object_size __dest,
		const char *__restrict __src, size_t __len))
     __warn_if_src_too_large (__dest, __src)
{
  return __builtin___strncat_chk (__dest, __src, __len,
				  __glibc_objsize (__dest));
}

#undef __warn_len_too_large
#undef __size_too_small
#undef __warn_if_dest_too_small
#undef __warn_if_dest_too_small0
#undef __warn_input_str_too_large
#undef __warn_if_src_too_large
#endif /* bits/string_fortified.h */
