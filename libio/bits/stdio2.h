/* Checking macros for stdio functions.
   Copyright (C) 2004-2023 Free Software Foundation, Inc.
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

#ifndef _BITS_STDIO2_H
#define _BITS_STDIO2_H 1

#ifndef _STDIO_H
# error "Never include <bits/stdio2.h> directly; use <stdio.h> instead."
#endif

#define __mul_may_overflow(size, n) \
  ((size | n) >= (((size_t)1) << (8 * sizeof (size_t) / 2)))

#ifdef __FORTIFY_ARG_PACK_OK
/* clang doesn't have __va_arg_pack, so we need to defer to the va_arg versions
   of these functions.  */
__fortify_potential_overload __attribute__ ((__format__ (__printf__, 2, 3))) int
__NTH (sprintf (char *__restrict const __clang_pass_object_size __s,
		const char *__restrict __fmt, ...))
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __result = __FORTIFY_CALL_VA_BUILTIN (sprintf, __s,
					    __USE_FORTIFY_LEVEL - 1,
					    __glibc_objsize (__s), __fmt,
					    __FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __result;
}
#elif !defined __cplusplus
# define sprintf(str, ...) \
  __builtin___sprintf_chk (str, __USE_FORTIFY_LEVEL - 1,		      \
			   __glibc_objsize (str), __VA_ARGS__)
#endif

__fortify_potential_overload int
__NTH (vsprintf (char *__restrict const __clang_pass_object_size __s,
		 const char *__restrict __fmt, __gnuc_va_list __ap))
{
  return __builtin___vsprintf_chk (__s, __USE_FORTIFY_LEVEL - 1,
				   __glibc_objsize (__s), __fmt, __ap);
}

#if defined __USE_ISOC99 || defined __USE_UNIX98
# ifdef __FORTIFY_ARG_PACK_OK
__fortify_potential_overload __attribute__ ((__format__ (__printf__, 3, 4))) int
__NTH (snprintf (char *__restrict const __clang_pass_object_size __s,
		 size_t __n, const char *__restrict __fmt, ...))
     /* GCC's builtin will catch this, so we just need to cover clang here.  */
     __clang_warning_if (__bos_static_lt (__n, __s),
			 "call to snprintf may overflow the destination buffer")
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __result = __FORTIFY_CALL_VA_BUILTIN (snprintf, __s, __n,
					    __USE_FORTIFY_LEVEL - 1,
					    __glibc_objsize (__s), __fmt,
					    __FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __result;
}
# elif !defined __cplusplus
#  define snprintf(str, len, ...) \
  __builtin___snprintf_chk (str, len, __USE_FORTIFY_LEVEL - 1,		      \
			    __glibc_objsize (str), __VA_ARGS__)
# endif

__fortify_potential_overload __attribute__ ((__format__ (__printf__, 3, 0))) int
__NTH (vsnprintf (char *__restrict const __clang_pass_object_size __s,
		  size_t __n, const char *__restrict __fmt, __gnuc_va_list __ap))
     __clang_warning_if (__bos_static_lt (__n, __s),
                         "call to vsnprintf may overflow the destination "
                         "buffer")
{
  return __builtin___vsnprintf_chk (__s, __n, __USE_FORTIFY_LEVEL - 1,
				    __glibc_objsize (__s), __fmt, __ap);
}

#endif

#if __USE_FORTIFY_LEVEL > 1
# ifdef __FORTIFY_ARG_PACK_OK
__fortify_potential_overload __attribute__ ((__format__ (__printf__, 2, 3))) int
fprintf (FILE *__restrict const __clang_pass_object_size __stream,
	 const char *__restrict __fmt, ...)
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __result = __FORTIFY_CALL_VA_CHK (fprintf, __stream,
					__USE_FORTIFY_LEVEL - 1, __fmt,
					__FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __result;
}

__fortify_potential_overload __attribute__ ((__format__ (__printf__, 1, 2))) int
printf (const char *__restrict const __clang_pass_object_size __fmt, ...)
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __result = __FORTIFY_CALL_VA_CHK (printf, __USE_FORTIFY_LEVEL - 1, __fmt,
					__FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __result;
}
# elif !defined __cplusplus
#  define printf(...) \
  __printf_chk (__USE_FORTIFY_LEVEL - 1, __VA_ARGS__)
#  define fprintf(stream, ...) \
  __fprintf_chk (stream, __USE_FORTIFY_LEVEL - 1, __VA_ARGS__)
# endif

__fortify_potential_overload __attribute__ ((__format__ (__printf__, 1, 0))) int
vprintf (const char *__restrict const __clang_pass_object_size __fmt,
	 __gnuc_va_list __ap)
{
# ifdef __USE_EXTERN_INLINES
  return __vfprintf_chk (stdout, __USE_FORTIFY_LEVEL - 1, __fmt, __ap);
# else
  return __vprintf_chk (__USE_FORTIFY_LEVEL - 1, __fmt, __ap);
# endif
}

__fortify_potential_overload __attribute__ ((__format__ (__printf__, 2, 0))) int
vfprintf (FILE *__restrict const __clang_pass_object_size __stream,
	  const char *__restrict __fmt, __gnuc_va_list __ap)
{
  return __vfprintf_chk (__stream, __USE_FORTIFY_LEVEL - 1, __fmt, __ap);
}

# ifdef __USE_XOPEN2K8
#  ifdef __FORTIFY_ARG_PACK_OK
__fortify_potential_overload __attribute__ ((__format__ (__printf__, 2, 3))) int
dprintf (int __fd, const char *__restrict const __clang_pass_object_size __fmt,
	 ...)
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __result = __FORTIFY_CALL_VA_CHK (dprintf, __fd, __USE_FORTIFY_LEVEL - 1,
					__fmt, __FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __result;
}
#  elif !defined __cplusplus
#   define dprintf(fd, ...) \
  __dprintf_chk (fd, __USE_FORTIFY_LEVEL - 1, __VA_ARGS__)
#  endif

__fortify_function int
vdprintf (int __fd, const char *__restrict __fmt, __gnuc_va_list __ap)
{
  return __vdprintf_chk (__fd, __USE_FORTIFY_LEVEL - 1, __fmt, __ap);
}
# endif

# ifdef __USE_GNU
#  ifdef __FORTIFY_ARG_PACK_OK
__fortify_potential_overload __attribute__ ((__format__ (__printf__, 2, 3)))
__wur int
__NTH (asprintf (char **__restrict const __clang_pass_object_size __ptr,
		 const char *__restrict __fmt, ...))
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __result = __FORTIFY_CALL_VA_CHK (asprintf, __ptr,
					__USE_FORTIFY_LEVEL - 1, __fmt,
					__FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __result;
}

__fortify_potential_overload __attribute__ ((__format__ (__printf__, 2, 3)))
__wur int
__NTH (__asprintf (char **__restrict const __clang_pass_object_size __ptr,
		   const char *__restrict __fmt, ...))
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __result = __FORTIFY_CALL_VA_CHK (asprintf, __ptr,
					__USE_FORTIFY_LEVEL - 1, __fmt,
					__FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __result;
}

__fortify_potential_overload __attribute__ ((__format__ (__printf__, 2, 3))) int
__NTH (obstack_printf (struct obstack *
			 __restrict const __clang_pass_object_size __obstack,
 		       const char *__restrict __fmt, ...))
{
  __FORTIFY_INIT_ARG_PACK(__fmt);
  int __result =
#   ifdef __use_clang_fortify
    __obstack_vprintf_chk
#   else
    __obstack_printf_chk
#   endif
      (__obstack, __USE_FORTIFY_LEVEL - 1, __fmt,
			     __FORTIFY_ARG_PACK);
  __FORTIFY_FREE_ARG_PACK();
  return __result;
}
#  elif !defined __cplusplus
#   define asprintf(ptr, ...) \
  __asprintf_chk (ptr, __USE_FORTIFY_LEVEL - 1, __VA_ARGS__)
#   define __asprintf(ptr, ...) \
  __asprintf_chk (ptr, __USE_FORTIFY_LEVEL - 1, __VA_ARGS__)
#   define obstack_printf(obstack, ...) \
  __obstack_printf_chk (obstack, __USE_FORTIFY_LEVEL - 1, __VA_ARGS__)
#  endif

__fortify_potential_overload __attribute__ ((__format__ (__printf__, 2, 0)))
__wur int
__NTH (vasprintf (char **__restrict const __clang_pass_object_size __ptr,
		  const char *__restrict __fmt, __gnuc_va_list __ap))
{
  return __vasprintf_chk (__ptr, __USE_FORTIFY_LEVEL - 1, __fmt, __ap);
}

__fortify_potential_overload __attribute__ ((__format__ (__printf__, 2, 0))) int
__NTH (obstack_vprintf (struct obstack *
			__restrict const __clang_pass_object_size __obstack,
			const char *__restrict __fmt, __gnuc_va_list __ap))
{
  return __obstack_vprintf_chk (__obstack, __USE_FORTIFY_LEVEL - 1, __fmt,
				__ap);
}

# endif

#endif

#if __GLIBC_USE (DEPRECATED_GETS)
extern char *__REDIRECT_NTH (__gets_alias, (char *__buf), gets) __wur;

__fortify_potential_overload __wur char *
gets (char *const __clang_pass_object_size __str)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_IF (__gets_warn, __glibc_objsize (__str) == (size_t) -1,
			   "please use fgets or getline instead, gets can't "
			   "specify buffer size")
{
  if (__glibc_objsize (__str) != (size_t) -1)
    return __gets_chk (__str, __glibc_objsize (__str));
  return __gets_alias (__str);
}
__FORTIFY_FUNCTION_END
#endif

extern char *__REDIRECT (__fgets_alias,
			 (char *__restrict __s, int __n,
			  FILE *__restrict __stream), fgets)
    __wur __attr_access ((__write_only__, 1, 2));

__fortify_potential_overload __wur  __attr_access ((__write_only__, 1, 2)) char *
fgets (char *__restrict const __clang_pass_object_size __s, int __n,
       FILE *__restrict __stream)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_IF (__fgets_warn, __bos_static_lt (__n, __s) && __n > 0,
			   "fgets called with bigger size than length of "
			   "destination buffer")
{
  if (__glibc_objsize (__s) != (size_t) -1)
    return __fgets_chk (__s, __glibc_objsize (__s), __n, __stream);
  return __fgets_alias (__s, __n, __stream);
}
__FORTIFY_FUNCTION_END

extern size_t __REDIRECT (__fread_alias,
			  (void *__restrict __ptr, size_t __size,
			   size_t __n, FILE *__restrict __stream),
			  fread) __wur;

__fortify_potential_overload __wur size_t
fread (void *__restrict const __clang_pass_object_size0 __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_IF (__fread_warn, __bos0_static_lt (__size * __n, __ptr)
			    && !__mul_may_overflow (__size, __n),
			   "fread called with bigger size * nmemb than length "
			   "of destination buffer")
{
  if (__glibc_objsize0 (__ptr) != (size_t) -1)
    return __fread_chk (__ptr, __glibc_objsize0 (__ptr), __size, __n, __stream);
  return __fread_alias (__ptr, __size, __n, __stream);
}
__FORTIFY_FUNCTION_END

#ifdef __USE_GNU
extern char *__REDIRECT (__fgets_unlocked_alias,
			 (char *__restrict __s, int __n,
			  FILE *__restrict __stream), fgets_unlocked)
    __wur __attr_access ((__write_only__, 1, 2));

__fortify_potential_overload __wur __attr_access ((__write_only__, 1, 2)) char *
fgets_unlocked (char *__restrict const __clang_pass_object_size __s, int __n,
		FILE *__restrict __stream)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_IF (__fgets_unlocked_warn,
			   __bos_static_lt (__n, __s) && __n > 0,
			   "fgets_unlocked called with bigger size than length "
			   "of destination buffer")
{
  if (__glibc_objsize (__s) != (size_t) -1)
    return __fgets_unlocked_chk (__s, __glibc_objsize (__s), __n, __stream);
  return __fgets_unlocked_alias (__s, __n, __stream);
}
__FORTIFY_FUNCTION_END
#endif

#ifdef __USE_MISC
# undef fread_unlocked
extern size_t __REDIRECT (__fread_unlocked_alias,
			  (void *__restrict __ptr, size_t __size,
			   size_t __n, FILE *__restrict __stream),
			  fread_unlocked) __wur;

__fortify_potential_overload __wur size_t
fread_unlocked (void *__restrict const __clang_pass_object_size0 __ptr,
		size_t __size, size_t __n, FILE *__restrict __stream)
__FORTIFY_PRECONDITIONS
     __FORTIFY_WARNING_IF (__fread_unlocked_warn,
			   __bos0_static_lt (__size * __n, __ptr)
			    && !__mul_may_overflow(__size, __n),
			   "fread_unlocked called with bigger size * n than "
			   "length of destination buffer")
{
  if (__glibc_objsize0 (__ptr) != (size_t) -1)
    return __fread_unlocked_chk (__ptr, __glibc_objsize0 (__ptr), __size, __n, __stream);

# ifdef __USE_EXTERN_INLINES
      if (__builtin_constant_p (__size)
	  && __builtin_constant_p (__n)
	  && (__size | __n) < (((size_t) 1) << (8 * sizeof (size_t) / 2))
	  && __size * __n <= 8)
	{
	  size_t __cnt = __size * __n;
	  char *__cptr = (char *) __ptr;
	  if (__cnt == 0)
	    return 0;

	  for (; __cnt > 0; --__cnt)
	    {
	      int __c = getc_unlocked (__stream);
	      if (__c == EOF)
		break;
	      *__cptr++ = __c;
	    }
	  return (__cptr - (char *) __ptr) / __size;
	}
# endif
      return __fread_unlocked_alias (__ptr, __size, __n, __stream);
}
__FORTIFY_FUNCTION_END
#endif

#endif /* bits/stdio2.h.  */
