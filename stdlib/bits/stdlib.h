/* Checking macros for stdlib functions.
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

#ifndef _STDLIB_H
# error "Never include <bits/stdlib.h> directly; use <stdlib.h> instead."
#endif

extern char *__realpath_chk (const char *__restrict __name,
			     char *__restrict __resolved,
			     size_t __resolvedlen) __THROW __wur;
extern char *__REDIRECT_NTH (__realpath_alias,
			     (const char *__restrict __name,
			      char *__restrict __resolved), realpath) __wur;
__fortify_potential_overload __wur char *
__NTH (realpath (const char *__restrict __name,
		 char *__restrict const __clang_pass_object_size __resolved))
__FORTIFY_PRECONDITIONS
#if defined _LIBC_LIMITS_H_ && defined PATH_MAX
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__realpath_warn, PATH_MAX, __resolved,
				       "second argument of realpath must be "
				       "either NULL or at least PATH_MAX "
				       "bytes long buffer")
#endif
{
  if (
#if defined _LIBC_LIMITS_H_ && defined PATH_MAX
      __FORTIFY_CALL_CHK &&
#endif
      __glibc_objsize (__resolved) != (size_t) -1)
    return __realpath_chk (__name, __resolved, __glibc_objsize (__resolved));
  return __realpath_alias (__name, __resolved);
}
__FORTIFY_FUNCTION_END


extern int __ptsname_r_chk (int __fd, char *__buf, size_t __buflen,
			    size_t __nreal) __THROW __nonnull ((2))
    __attr_access ((__write_only__, 2, 3));
extern int __REDIRECT_NTH (__ptsname_r_alias, (int __fd, char *__buf,
					       size_t __buflen), ptsname_r)
     __nonnull ((2)) __attr_access ((__write_only__, 2, 3));

__fortify_potential_overload int
__NTH (ptsname_r (int __fd, char *const __clang_pass_object_size __buf,
		  size_t __buflen))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__ptsname_r_warn, __buflen, __buf,
				       "ptsname_r called with buflen "
				       "bigger than size of buf")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__buf) != (size_t) -1)
    return __ptsname_r_chk (__fd, __buf, __buflen, __glibc_objsize (__buf));
  return __ptsname_r_alias (__fd, __buf, __buflen);
}
__FORTIFY_FUNCTION_END

extern int __wctomb_chk (char *__s, wchar_t __wchar, size_t __buflen)
  __THROW __wur;
extern int __REDIRECT_NTH (__wctomb_alias, (char *__s, wchar_t __wchar),
			   wctomb) __wur;

__fortify_potential_overload __wur int
__NTH (wctomb (char *const __clang_pass_object_size __s, wchar_t __wchar))
{
  /* We would have to include <limits.h> to get a definition of MB_LEN_MAX.
     But this would only disturb the namespace.  So we define our own
     version here.  */
#define __STDLIB_MB_LEN_MAX	16
#if defined MB_LEN_MAX && MB_LEN_MAX != __STDLIB_MB_LEN_MAX
# error "Assumed value of MB_LEN_MAX wrong"
#endif
  if (__glibc_objsize (__s) != (size_t) -1
      && __STDLIB_MB_LEN_MAX > __glibc_objsize (__s))
    return __wctomb_chk (__s, __wchar, __glibc_objsize (__s));
  return __wctomb_alias (__s, __wchar);
}


extern size_t __mbstowcs_chk (wchar_t *__restrict __dst,
			      const char *__restrict __src,
			      size_t __len, size_t __dstlen) __THROW
    __attr_access ((__write_only__, 1, 3)) __attr_access ((__read_only__, 2));
extern size_t __REDIRECT_NTH (__mbstowcs_nulldst,
			      (wchar_t *__restrict __dst,
			       const char *__restrict __src,
			       size_t __len), mbstowcs)
    __attr_access ((__read_only__, 2));
extern size_t __REDIRECT_NTH (__mbstowcs_alias,
			      (wchar_t *__restrict __dst,
			       const char *__restrict __src,
			       size_t __len), mbstowcs)
    __attr_access ((__write_only__, 1, 3)) __attr_access ((__read_only__, 2));

__fortify_potential_overload size_t
__NTH (mbstowcs (wchar_t *__restrict const __clang_pass_object_size __dst,
		 const char *__restrict __src, size_t __len))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT2 (__mbstowcs_warn, __len, __dst,
					sizeof (wchar_t),
					"mbstowcs called with dst buffer "
					"smaller than len * sizeof (wchar_t)")
{
  if (__builtin_constant_p (__dst == NULL) && __dst == NULL)
    return __mbstowcs_nulldst (__dst, __src, __len);
  else if (__FORTIFY_CALL_CHK && __glibc_objsize (__dst) != (size_t) -1)
    return __mbstowcs_chk (__dst, __src, __len,
			   __glibc_objsize (__dst) / sizeof (wchar_t));
  return __mbstowcs_alias (__dst, __src, __len);
}
__FORTIFY_FUNCTION_END

extern size_t __wcstombs_chk (char *__restrict __dst,
			      const wchar_t *__restrict __src,
			      size_t __len, size_t __dstlen) __THROW
  __attr_access ((__write_only__, 1, 3)) __attr_access ((__read_only__, 2));
extern size_t __REDIRECT_NTH (__wcstombs_alias,
			      (char *__restrict __dst,
			       const wchar_t *__restrict __src,
			       size_t __len), wcstombs)
  __attr_access ((__write_only__, 1, 3)) __attr_access ((__read_only__, 2));

__fortify_potential_overload size_t
__NTH (wcstombs (char *__restrict const __clang_pass_object_size __dst,
		 const wchar_t *__restrict __src, size_t __len))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__wcstombs_warn, __len, __dst,
				       "wcstombs called with dst buffer "
				       "smaller than len")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__dst) != (size_t) -1)
    return __wcstombs_chk (__dst, __src, __len, __glibc_objsize (__dst));
  return __wcstombs_alias (__dst, __src, __len);
}
__FORTIFY_FUNCTION_END
