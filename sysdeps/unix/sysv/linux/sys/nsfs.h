/* Namespace file system interfaces.  Linux version.
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

#ifndef _SYS_NSFS_H
#define _SYS_NSFS_H	1

#include <features.h>
#include <bits/types.h>
#define __need_size_t
#include <stddef.h>

#ifdef __has_include
# if __has_include ("linux/nsfs.h")
#  include "linux/nsfs.h"
# endif
#endif

#ifndef NS_ID_REQ_SIZE_VER0
/* Namespace and parameters for listns.  NS_ID is the last listed namespace ID
   (or zero), NS_TYPE is a mask of CLONE_NEW* values to select the namespace
   types (or zero for all of them), and USER_NS_ID selects only the namespaces
   owned by that user namespace (or zero for any owner).  */
struct ns_id_req
{
  __uint32_t size;
  __uint32_t spare;
  __uint64_t ns_id;
  __extension__ struct
  {
    __uint32_t ns_type;
    __uint32_t spare2;
    __uint64_t user_ns_id;
  };
};

/* Special USER_NS_ID value for listns: the user namespace of the
   caller.  */
# define LISTNS_CURRENT_USER	0xffffffffffffffff

# define NS_ID_REQ_SIZE_VER0	32 /* sizeof first published struct */
#endif

#ifndef __ssize_t_defined
typedef __ssize_t ssize_t;
# define __ssize_t_defined
#endif

__BEGIN_DECLS

/* Store in NS_IDS the IDs of up to NR_NS_IDS namespaces selected by REQ, in
   ascending order and starting after REQ->ns_id.  Only active namespaces the
   caller is allowed to see are listed.  FLAGS must be zero.
   Return the number of IDs stored, or -1 and set errno on failure.  */
extern ssize_t listns (const struct ns_id_req *__req, __uint64_t *__ns_ids,
		       size_t __nr_ns_ids, unsigned int __flags) __THROW;

__END_DECLS

#endif /* sys/nsfs.h */
