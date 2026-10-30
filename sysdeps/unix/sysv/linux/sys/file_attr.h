/* File attributes.  Linux version.
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

#ifndef _SYS_FILE_ATTR_H
#define _SYS_FILE_ATTR_H	1

#include <features.h>
#include <bits/types.h>
#include <stddef.h>

#ifdef __has_include
# if __has_include ("linux/fs.h")
#  include "linux/fs.h"
# endif
#endif

#ifndef FILE_ATTR_SIZE_VER0
/* Attributes for file_getattr and file_setattr.  */
struct file_attr
{
  __uint64_t fa_xflags;		/* FS_XFLAG_* flags (get/set).  */
  __uint32_t fa_extsize;	/* Extent size hint (get/set).  */
  __uint32_t fa_nextents;	/* Number of extents (get).  */
  __uint32_t fa_projid;		/* Project identifier (get/set).  */
  __uint32_t fa_cowextsize;	/* CoW extent size hint (get/set).  */
};
# define FILE_ATTR_SIZE_VER0 24
# define FILE_ATTR_SIZE_LATEST FILE_ATTR_SIZE_VER0
#endif

/* Flags for the fa_xflags field.  */
#define FS_XFLAG_REALTIME	0x00000001	/* Data in realtime volume.  */
#define FS_XFLAG_PREALLOC	0x00000002	/* Preallocated extents.  */
#define FS_XFLAG_IMMUTABLE	0x00000008	/* File cannot be modified.  */
#define FS_XFLAG_APPEND		0x00000010	/* All writes append.  */
#define FS_XFLAG_SYNC		0x00000020	/* All writes synchronous.  */
#define FS_XFLAG_NOATIME	0x00000040	/* No access time updates.  */
#define FS_XFLAG_NODUMP		0x00000080	/* Not included in backups.  */
#define FS_XFLAG_RTINHERIT	0x00000100	/* Create with rt bit set.  */
#define FS_XFLAG_PROJINHERIT	0x00000200	/* Inherit parent projid.  */
#define FS_XFLAG_NOSYMLINKS	0x00000400	/* No symlink creation.  */
#define FS_XFLAG_EXTSIZE	0x00000800	/* Extent size hint.  */
#define FS_XFLAG_EXTSZINHERIT	0x00001000	/* Inherit extent size.  */
#define FS_XFLAG_NODEFRAG	0x00002000	/* Do not defragment.  */
#define FS_XFLAG_FILESTREAM	0x00004000	/* Filestream allocator.  */
#define FS_XFLAG_DAX		0x00008000	/* Use DAX for IO.  */
#define FS_XFLAG_COWEXTSIZE	0x00010000	/* CoW extent size hint.  */
#define FS_XFLAG_VERITY		0x00020000	/* fs-verity enabled.  */
#define FS_XFLAG_CASEFOLD	0x00040000	/* Case-insensitive lookups
						   (read-only).  */
#define FS_XFLAG_CASENONPRESERVING 0x00080000	/* Case not preserved
						   (read-only).  */
#define FS_XFLAG_HASATTR	0x80000000	/* No DIFLAG for this.  */

__BEGIN_DECLS

/* The following functions operate on the file pointed to by PATH, which is
   relative to the directory DFD (as openat).  AT_FLAGS can contain
   AT_SYMLINK_NOFOLLOW and AT_EMPTY_PATH.   With AT_EMPTY_PATH, PATH can be
   empty or a null pointer to operate on DFD itself.  SIZE is the size of
   the structure pointed to by FATTR.  */

/* Store in FATTR the attributes of the file.  Return 0 on success, -1 for
   errors.  */
extern int file_getattr (int __dfd, const char *__path,
			 struct file_attr *__fattr, size_t __size,
			 unsigned int __at_flags) __THROW;

/* Set the attributes of the file to FATTR.  Return 0 on success, -1 for
   errors.  */
extern int file_setattr (int __dfd, const char *__path,
			 const struct file_attr *__fattr, size_t __size,
			 unsigned int __at_flags) __THROW;

__END_DECLS

#endif /* sys/file_attr.h */
