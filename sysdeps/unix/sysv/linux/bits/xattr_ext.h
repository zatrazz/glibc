/* System-specific extensions of <sys/xattr.h>, Linux version.
   Copyright (C) 2026 Free Software Foundation, Inc.
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

#ifndef _SYS_XATTR_H
# error "Never include <bits/xattr_ext.h> directly; use <sys/xattr.h> instead."
#endif

#ifndef __USE_KERNEL_XATTR_DEFS
struct xattr_args
{
  __uint64_t value __attribute__ ((__aligned__ (8)));
				/* Address of the attribute value buffer.  */
  __uint32_t size;	/* Size of the value (or of the buffer).  */
  __uint32_t flags;	/* XATTR_CREATE or XATTR_REPLACE for setxattrat,
			   zero for getxattrat.  */
};
#endif

/* Kernel headers older than Linux 6.13 do not define it.  */
struct xattr_args;

/* The following functions operate on the file pointed to by PATH, which is
   relative to the directory DFD.  AT_FLAGS can contain AT_SYMLINK_NOFOLLOW
   and AT_EMPTY_PATH, and it can be empty or a null pointer to operate on DFD
   itself.  */

/* Set the attribute NAME to the value described by ARGS, of size SIZE.
   Return 0 on success, -1 for errors.  */
extern int setxattrat (int __dfd, const char *__path, unsigned int __at_flags,
		       const char *__name, const struct xattr_args *__args,
		       size_t __size) __THROW;

/* Get the attribute NAME into the buffer described by ARGS, of size SIZE.
   Return the size of the attribute value on success, -1 for errors.  */
extern ssize_t getxattrat (int __dfd, const char *__path,
			   unsigned int __at_flags, const char *__name,
			   struct xattr_args *__args, size_t __size) __THROW;

/* Store in LIST, of size SIZE, the list of attribute names.  Return the
   size of the list on success, -1 for errors.  */
extern ssize_t listxattrat (int __dfd, const char *__path,
			    unsigned int __at_flags, char *__list,
			    size_t __size) __THROW;

/* Remove the attribute NAME.  Return 0 on success, -1 for errors.  */
extern int removexattrat (int __dfd, const char *__path,
			  unsigned int __at_flags, const char *__name)
     __THROW;
