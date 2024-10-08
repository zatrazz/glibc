/* Fortify macros for strings.h functions.
   Copyright (C) 2017-2023 Free Software Foundation, Inc.
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

#ifndef __STRINGS_FORTIFIED
# define __STRINGS_FORTIFIED 1

#define __strings_warn_len_too_large \
  "function called with bigger length than the destination buffer"

#define __strings_size_too_small(dest, len) \
  (__bos0 (dest) != (size_t) -1 && __bos0 (dest) < len)

__fortify_potential_overload void
__NTH (bcopy (const void *__src, void *const __clang_pass_object_size0 __dest,
	      size_t __len))
     __clang_warning_if (__strings_size_too_small (__dest, __len),
			 __strings_warn_len_too_large)
{
  size_t __glibc_objsize_dst = __glibc_objsize0 (__dest);
  if (__glibc_objsize_dst == (size_t) -1 || (__builtin_constant_p (__len)
				   && __glibc_objsize_dst >= __len))
    (void) __builtin_memmove (__dest, __src, __len);
  else
    (void) __builtin___memmove_chk (__dest, __src, __len, __glibc_objsize_dst);
}

__fortify_potential_overload void
__NTH (bzero (void *const __clang_pass_object_size0 __dest, size_t __len))
     __clang_warning_if (__strings_size_too_small (__dest, __len),
			 __strings_warn_len_too_large)
{
  size_t __glibc_objsize_dst = __glibc_objsize0 (__dest);
  if (__glibc_objsize_dst == (size_t) -1 || (__builtin_constant_p (__len)
				   && __glibc_objsize_dst >= __len))
    (void) __builtin_memset (__dest, '\0', __len);
  else
    (void) __builtin___memset_chk (__dest, '\0', __len, __glibc_objsize_dst);
}


#undef __strings_size_too_small
#undef __strings_warn_len_too_large
#endif
