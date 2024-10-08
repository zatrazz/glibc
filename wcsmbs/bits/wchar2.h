/* Checking macros for wchar functions.
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

#ifndef _WCHAR_H
# error "Never include <bits/wchar2.h> directly; use <wchar.h> instead."
#endif

extern wchar_t *__REDIRECT_NTH (__wmemcpy_alias,
				(wchar_t *__restrict __s1,
				 const wchar_t *__restrict __s2, size_t __n),
				wmemcpy);

__fortify_potential_overload wchar_t *
__NTH (wmemcpy (wchar_t *__restrict const __clang_pass_object_size0 __s1,
	      const wchar_t *__restrict __s2, size_t __n))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS0_LT2 (__wmemcpy_warn, __n, __s1,
					 sizeof (wchar_t),
					 "wmemcpy called with length bigger "
					 "than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize0 (__s1) != (size_t)-1)
    return __wmemcpy_chk(__s1, __s2, __n,
			 __glibc_objsize0(__s1) / sizeof (wchar_t));
  return __wmemcpy_alias(__s1, __s2, __n);
}
__FORTIFY_FUNCTION_END

extern wchar_t *__REDIRECT_NTH (__wmemmove_alias, (wchar_t *__s1,
						   const wchar_t *__s2,
						   size_t __n), wmemmove);

__fortify_potential_overload wchar_t *
__NTH (wmemmove (wchar_t *const __clang_pass_object_size0 __s1,
		 const wchar_t *__s2, size_t __n))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS0_LT2 (__wmemmove_warn, __n, __s1,
					 sizeof (wchar_t),
					 "wmemmove called with length bigger "
					 "than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize0 (__s1) != (size_t) -1)
    return __wmemmove_chk (__s1, __s2, __n,
			   __glibc_objsize0 (__s1) / sizeof (wchar_t));
  return __wmemmove_alias (__s1, __s2, __n);
}
__FORTIFY_FUNCTION_END


#ifdef __USE_GNU
extern wchar_t *__REDIRECT_NTH (__wmempcpy_alias,
				(wchar_t *__restrict __s1,
				 const wchar_t *__restrict __s2,
				 size_t __n), wmempcpy);

__fortify_potential_overload wchar_t *
__NTH(wmempcpy(wchar_t *__restrict const __clang_pass_object_size0 __s1,
               const wchar_t *__restrict __s2, size_t __n))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS0_LT2 (__wmempcpy_warn, __n, __s1,
					 sizeof (wchar_t),
					 "wmempcpy called with length bigger "
					 "than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize0 (__s1) != (size_t)-1)
    return __wmempcpy_chk(__s1, __s2, __n,
			  __glibc_objsize0(__s1) / sizeof (wchar_t));
  return __wmempcpy_alias(__s1, __s2, __n);
}
__FORTIFY_FUNCTION_END
#endif


extern wchar_t *__REDIRECT_NTH (__wmemset_alias, (wchar_t *__s, wchar_t __c,
						  size_t __n), wmemset);

__fortify_potential_overload wchar_t *
__NTH (wmemset (wchar_t *const __clang_pass_object_size0 __s, wchar_t __c,
		size_t __n))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS0_LT2 (__wmemset_warn, __n, __s,
					 sizeof (wchar_t),
					 "wmemset called with length bigger "
					 "than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize0 (__s) != (size_t) -1)
    return __wmemset_chk (__s, __c, __n,
			  __glibc_objsize0 (__s) / sizeof (wchar_t));
  return __wmemset_alias (__s, __c, __n);
}
__FORTIFY_FUNCTION_END


extern wchar_t *__REDIRECT_NTH (__wcscpy_alias,
				(wchar_t *__restrict __dest,
				 const wchar_t *__restrict __src), wcscpy);

__fortify_potential_overload wchar_t *
__NTH (wcscpy (wchar_t *__restrict const __clang_pass_object_size __dest,
	       const wchar_t *__restrict __src))
{
  size_t __sz = __glibc_objsize (__dest);
  if (__sz != (size_t) -1)
    return __wcscpy_chk (__dest, __src, __sz / sizeof (wchar_t));
  return __wcscpy_alias (__dest, __src);
}


extern wchar_t *__REDIRECT_NTH (__wcpcpy_alias,
				(wchar_t *__restrict __dest,
				 const wchar_t *__restrict __src), wcpcpy);

__fortify_potential_overload wchar_t *
__NTH (wcpcpy (wchar_t *__restrict const __clang_pass_object_size __dest,
	       const wchar_t *__restrict __src))
{
  size_t __sz = __glibc_objsize (__dest);
  if (__sz != (size_t) -1)
    return __wcpcpy_chk (__dest, __src, __sz / sizeof (wchar_t));
  return __wcpcpy_alias (__dest, __src);
}


extern wchar_t *__REDIRECT_NTH (__wcsncpy_alias,
				(wchar_t *__restrict __dest,
				 const wchar_t *__restrict __src,
				 size_t __n), wcsncpy);

__fortify_potential_overload wchar_t *
__NTH (wcsncpy (wchar_t *__restrict const __clang_pass_object_size __dest,
		const wchar_t *__restrict __src, size_t __n))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT2 (__wcsncpy_warn, __n, __dest,
					sizeof (wchar_t),
					"wcsncpy called with length bigger "
					"than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__dest) != (size_t) -1)
    return __wcsncpy_chk (__dest, __src, __n,
			  __glibc_objsize (__dest) / sizeof (wchar_t));
  return __wcsncpy_alias (__dest, __src, __n);
}
__FORTIFY_FUNCTION_END


extern wchar_t *__REDIRECT_NTH (__wcpncpy_alias,
				(wchar_t *__restrict __dest,
				 const wchar_t *__restrict __src,
				 size_t __n), wcpncpy);

__fortify_potential_overload wchar_t *
__NTH (wcpncpy (wchar_t *__restrict const __clang_pass_object_size __dest,
		const wchar_t *__restrict __src, size_t __n))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT2 (__wcpncpy_warn, __n, __dest,
					sizeof (wchar_t),
					"wcpncpy called with length bigger "
					"than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__dest) != (size_t) -1)
    return __wcpncpy_chk (__dest, __src, __n,
			  __glibc_objsize (__dest) / sizeof (wchar_t));
  return __wcpncpy_alias (__dest, __src, __n);
}
__FORTIFY_FUNCTION_END

extern wchar_t *__REDIRECT_NTH (__wcscat_alias,
				(wchar_t *__restrict __dest,
				 const wchar_t *__restrict __src), wcscat);

__fortify_potential_overload wchar_t *
__NTH (wcscat (wchar_t *__restrict const __clang_pass_object_size __dest,
	       const wchar_t *__restrict __src))
{
  size_t __sz = __glibc_objsize (__dest);
  if (__sz != (size_t) -1)
    return __wcscat_chk (__dest, __src, __sz / sizeof (wchar_t));
  return __wcscat_alias (__dest, __src);
}


extern wchar_t *__REDIRECT_NTH (__wcsncat_alias,
				(wchar_t *__restrict __dest,
				 const wchar_t *__restrict __src,
				 size_t __n), wcsncat);

__fortify_potential_overload wchar_t *
__NTH (wcsncat (wchar_t *__restrict const __clang_pass_object_size __dest,
		const wchar_t *__restrict __src, size_t __n))
{
  size_t __sz = __glibc_objsize (__dest);
  if (__sz != (size_t) -1)
    return __wcsncat_chk (__dest, __src, __n, __sz / sizeof (wchar_t));
  return __wcsncat_alias (__dest, __src, __n);
}



extern int __REDIRECT_NTH_LDBL (__swprintf_alias,
				(wchar_t *__restrict __s, size_t __n,
				 const wchar_t *__restrict __fmt, ...),
				swprintf);

extern int __REDIRECT_NTH_LDBL (__vswprintf_alias,
				(wchar_t *__restrict __s, size_t __n,
				 const wchar_t *__restrict __fmt,
				 __gnuc_va_list __ap), vswprintf);

#ifdef __FORTIFY_ARG_PACK_OK
__fortify_potential_overload int
__NTH (swprintf (wchar_t *__restrict const __clang_pass_object_size __s,
		 size_t __n, const wchar_t *__restrict __fmt, ...))
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __result;
  if (__glibc_objsize (__s) != (size_t) -1 || __USE_FORTIFY_LEVEL > 1)
    __result = __FORTIFY_CALL_VA_CHK(swprintf, __s, __n,
				     __USE_FORTIFY_LEVEL - 1,
				     __glibc_objsize (__s) / sizeof (wchar_t), __fmt,
				     __FORTIFY_ARG_PACK);
  else
    __result = __FORTIFY_CALL_VA_ALIAS(swprintf, __s, __n, __fmt,
				       __FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __result;
}
#elif !defined __cplusplus
/* XXX We might want to have support in gcc for swprintf.  */
# define swprintf(s, n, ...) \
  (__glibc_objsize (s) != (size_t) -1 || __USE_FORTIFY_LEVEL > 1		      \
   ? __swprintf_chk (s, n, __USE_FORTIFY_LEVEL - 1,			      \
		     __glibc_objsize (s) / sizeof (wchar_t), __VA_ARGS__)	      \
   : swprintf (s, n, __VA_ARGS__))
#endif

__fortify_potential_overload int
__NTH (vswprintf (wchar_t *__restrict const __clang_pass_object_size __s,
		  size_t __n, const wchar_t *__restrict __fmt,
		  __gnuc_va_list __ap))
{
  size_t __sz = __glibc_objsize (__s);
  if (__sz != (size_t) -1 || __USE_FORTIFY_LEVEL > 1)
    return __vswprintf_chk (__s, __n,  __USE_FORTIFY_LEVEL - 1,
			    __sz / sizeof (wchar_t), __fmt, __ap);
  return __vswprintf_alias (__s, __n, __fmt, __ap);
}


#if __USE_FORTIFY_LEVEL > 1

#ifdef __FORTIFY_ARG_PACK_OK
__fortify_potential_overload int
wprintf (const wchar_t *__restrict const __clang_pass_object_size __fmt, ...)
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __r = __FORTIFY_CALL_VA_CHK (wprintf, __USE_FORTIFY_LEVEL - 1, __fmt,
				   __FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __r;
}

__fortify_potential_overload int
fwprintf (__FILE *__restrict const __clang_pass_object_size __stream,
	  const wchar_t *__restrict __fmt, ...)
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __r = __FORTIFY_CALL_VA_CHK (fwprintf, __stream, __USE_FORTIFY_LEVEL - 1,
				   __fmt, __FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __r;
}
# elif !defined __cplusplus
#  define wprintf(...) \
  __wprintf_chk (__USE_FORTIFY_LEVEL - 1, __VA_ARGS__)
#  define fwprintf(stream, ...) \
  __fwprintf_chk (stream, __USE_FORTIFY_LEVEL - 1, __VA_ARGS__)
# endif

__fortify_potential_overload int
vwprintf (const wchar_t *__restrict const __clang_pass_object_size __fmt,
	  __gnuc_va_list __ap)
{
  return __vwprintf_chk (__USE_FORTIFY_LEVEL - 1, __fmt, __ap);
}

__fortify_potential_overload int
vfwprintf (__FILE *__restrict const __clang_pass_object_size __stream,
	   const wchar_t *__restrict __fmt, __gnuc_va_list __ap)
{
  return __vfwprintf_chk (__stream, __USE_FORTIFY_LEVEL - 1, __fmt, __ap);
}

#endif

extern wchar_t *__REDIRECT (__fgetws_alias,
			    (wchar_t *__restrict __s, int __n,
			     __FILE *__restrict __stream), fgetws) __wur;

__fortify_potential_overload __wur wchar_t *
fgetws (wchar_t *__restrict const __clang_pass_object_size __s, int __n,
	__FILE *__restrict __stream)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT2 (__fgetws_warn, __n, __s,
					sizeof (wchar_t),
					"fgetws called with length bigger "
					"than size of destination buffer")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__s) != (size_t) -1)
    return __fgetws_chk (__s, __glibc_objsize (__s) / sizeof (wchar_t),
			 __n, __stream);
  return __fgetws_alias (__s, __n, __stream);
}
__FORTIFY_FUNCTION_END

#ifdef __USE_GNU
extern wchar_t *__REDIRECT (__fgetws_unlocked_alias,
			    (wchar_t *__restrict __s, int __n,
			     __FILE *__restrict __stream), fgetws_unlocked)
  __wur;

__fortify_potential_overload __wur wchar_t *
fgetws_unlocked (wchar_t *__restrict const __clang_pass_object_size __s,
		 int __n, __FILE *__restrict __stream)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_IF (__fgetws_unlocked_warn,
			   __n > 0
			    && __bos_static_lt2 (__n, __s,
							   sizeof (wchar_t)),
			   "fgetws_unlocked called with bigger size than "
			   "length of destination buffer")
{
  if (__glibc_objsize (__s) != (size_t) -1)
    return __fgetws_unlocked_chk (__s, __glibc_objsize (__s) / sizeof (wchar_t),
				  __n, __stream);
  return __fgetws_unlocked_alias (__s, __n, __stream);
}
__FORTIFY_FUNCTION_END
#endif


extern size_t __REDIRECT_NTH (__wcrtomb_alias,
			      (char *__restrict __s, wchar_t __wchar,
			       mbstate_t *__restrict __ps), wcrtomb) __wur;

__fortify_potential_overload __wur size_t
__NTH (wcrtomb (char *__restrict const __clang_pass_object_size __s,
		wchar_t __wchar, mbstate_t *__restrict __ps))
{
  /* We would have to include <limits.h> to get a definition of MB_LEN_MAX.
     But this would only disturb the namespace.  So we define our own
     version here.  */
#define __WCHAR_MB_LEN_MAX	16
#if defined MB_LEN_MAX && MB_LEN_MAX != __WCHAR_MB_LEN_MAX
# error "Assumed value of MB_LEN_MAX wrong"
#endif
  if (__glibc_objsize (__s) != (size_t) -1
      && __WCHAR_MB_LEN_MAX > __glibc_objsize (__s))
    return __wcrtomb_chk (__s, __wchar, __ps, __glibc_objsize (__s));
  return __wcrtomb_alias (__s, __wchar, __ps);
}


extern size_t __REDIRECT_NTH (__mbsrtowcs_alias,
			      (wchar_t *__restrict __dst,
			       const char **__restrict __src,
			       size_t __len, mbstate_t *__restrict __ps),
			      mbsrtowcs);

__fortify_potential_overload size_t
__NTH (mbsrtowcs (wchar_t *__restrict const __clang_pass_object_size __dst,
		  const char **__restrict __src, size_t __len,
		  mbstate_t *__restrict __ps))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT2 (__mbsrtowcs_warn, __len, __dst,
					sizeof (wchar_t),
					"mbsrtowcs called with dst buffer "
					"smaller than len * sizeof (wchar_t)")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__dst) != (size_t) -1)
    return __mbsrtowcs_chk (__dst, __src, __len, __ps,
			    __glibc_objsize (__dst) / sizeof (wchar_t));
  return __mbsrtowcs_alias (__dst, __src, __len, __ps);
}
__FORTIFY_FUNCTION_END


extern size_t __REDIRECT_NTH (__wcsrtombs_alias,
			      (char *__restrict __dst,
			       const wchar_t **__restrict __src,
			       size_t __len, mbstate_t *__restrict __ps),
			      wcsrtombs);

__fortify_potential_overload size_t
__NTH (wcsrtombs (char *__restrict const __clang_pass_object_size __dst,
		  const wchar_t **__restrict __src, size_t __len,
		  mbstate_t *__restrict __ps))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__wcsrtombs_warn, __len, __dst,
				       "wcsrtombs called with dst buffer "
				       "smaller than len")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__dst) != (size_t) -1)
    return __wcsrtombs_chk (__dst, __src, __len, __ps, __glibc_objsize (__dst));
  return __wcsrtombs_alias (__dst, __src, __len, __ps);
}
__FORTIFY_FUNCTION_END


#ifdef	__USE_XOPEN2K8
extern size_t __REDIRECT_NTH (__mbsnrtowcs_alias,
			      (wchar_t *__restrict __dst,
			       const char **__restrict __src, size_t __nmc,
			       size_t __len, mbstate_t *__restrict __ps),
			      mbsnrtowcs);

__fortify_potential_overload size_t
__NTH (mbsnrtowcs (wchar_t *__restrict const __clang_pass_object_size __dst,
		   const char **__restrict __src, size_t __nmc, size_t __len,
		   mbstate_t *__restrict __ps))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__mbsnrtowcs_warn,
				       sizeof (wchar_t) * __len, __dst,
				       "mbsnrtowcs called with dst buffer "
				       "smaller than len * sizeof (wchar_t)")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__dst) != (size_t) -1)
    return __mbsnrtowcs_chk (__dst, __src, __nmc, __len, __ps,
			     __glibc_objsize (__dst) / sizeof (wchar_t));
  return __mbsnrtowcs_alias (__dst, __src, __nmc, __len, __ps);
}
__FORTIFY_FUNCTION_END


extern size_t __REDIRECT_NTH (__wcsnrtombs_alias,
			      (char *__restrict __dst,
			       const wchar_t **__restrict __src,
			       size_t __nwc, size_t __len,
			       mbstate_t *__restrict __ps), wcsnrtombs);

__fortify_potential_overload size_t
__NTH (wcsnrtombs (char *__restrict const __clang_pass_object_size __dst,
		   const wchar_t **__restrict __src, size_t __nwc, size_t __len,
		   mbstate_t *__restrict __ps))
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_ONLY_IF_BOS_LT (__wcsnrtombs_warn, __len, __dst,
				       "wcsnrtombs called with dst buffer "
				       "smaller than len")
{
  if (__FORTIFY_CALL_CHK && __glibc_objsize (__dst) != (size_t) -1)
    return __wcsnrtombs_chk (__dst, __src, __nwc, __len, __ps,
			     __glibc_objsize (__dst));
  return __wcsnrtombs_alias (__dst, __src, __nwc, __len, __ps);
}
__FORTIFY_FUNCTION_END
#endif
