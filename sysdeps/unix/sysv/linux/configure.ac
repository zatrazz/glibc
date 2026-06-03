GLIBC_PROVIDES dnl See aclocal.m4 in the top level source directory.
# Local configure fragment for sysdeps/unix/sysv/linux.

define([LIBC_LINUX_VERSION],[3.2.0])dnl
if test -n "$sysheaders"; then
  OLD_CPPFLAGS=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS $SYSINCLUDES"
fi
define([libc_cv_linuxVER], [libc_cv_linux]patsubst(LIBC_LINUX_VERSION,[\.]))dnl
AC_CACHE_CHECK(installed Linux kernel header files, libc_cv_linuxVER, [dnl
AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[#include <linux/version.h>
#if !defined LINUX_VERSION_CODE || LINUX_VERSION_CODE < ]dnl
patsubst(LIBC_LINUX_VERSION,[^\([^.]*\)\.\([^.]*\)\.\([^.]*\)$],dnl
[ (\1 *65536+ \2 *256+ \3) /* \1.\2.\3 */])[
# error kernel headers missing or too old
#endif]], [[]])],
	       [libc_cv_linuxVER='LIBC_LINUX_VERSION or later'],
	       [libc_cv_linuxVER='missing or too old!'])])
if test "$libc_cv_linuxVER" != 'LIBC_LINUX_VERSION or later'; then
  AC_MSG_ERROR([GNU libc requires kernel header files from
Linux LIBC_LINUX_VERSION or later to be installed before configuring.
The kernel header files are found usually in /usr/include/asm and
/usr/include/linux; make sure these directories use files from
Linux LIBC_LINUX_VERSION or later.  This check uses <linux/version.h>, so
make sure that file was built correctly when installing the kernel header
files.  To use kernel headers not from /usr/include/linux, use the
configure option --with-headers.])
fi

# If the user gave a minimal version number test whether the available
# kernel headers are young enough.  Additionally we have minimal
# kernel versions for some architectures.  If a previous configure fragment
# set arch_minimum_kernel already, let that override our defaults here.
# Note that we presume such a fragment has set libc_cv_gcc_unwind_find_fde
# if appropriate too.
test -n "$arch_minimum_kernel" || arch_minimum_kernel=3.2.0
if test -n "$minimum_kernel"; then
  changequote(,)
  user_version=$((`echo "$minimum_kernel.0.0.0" | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\1 \* 65536 + \2 \* 256 + \3/'`))
  arch_version=$((`echo "$arch_minimum_kernel.0.0.0" | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\1 \* 65536 + \2 \* 256 + \3/'`))
  changequote([,])
  if test $user_version -lt $arch_version; then
    AC_MSG_WARN([minimum kernel version reset to $arch_minimum_kernel])
    minimum_kernel=$arch_minimum_kernel
  fi
else
  minimum_kernel=$arch_minimum_kernel
fi

AC_MSG_CHECKING(for kernel header at least $minimum_kernel)
changequote(,)dnl
decnum=`echo "$minimum_kernel.0.0.0" | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/(\1 * 65536 + \2 * 256 + \3)/'`;
abinumstr=`echo "$minimum_kernel.0.0.0" | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\1.\2.\3/'`;
abinum=`echo "$minimum_kernel.0.0.0" | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\1,\2,\3/'`;
changequote([,])dnl
AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[#include <linux/version.h>
#if LINUX_VERSION_CODE < $decnum
# error kernel headers too old
#endif]], [[]])], [libc_minimum_kernel=ok], [libc_minimum_kernel='too old!'])
AC_MSG_RESULT($libc_minimum_kernel)
if test "$libc_minimum_kernel" = ok; then
  AC_DEFINE_UNQUOTED(__LINUX_KERNEL_VERSION, $decnum)
  AC_DEFINE_UNQUOTED(__LINUX_KERNEL_VERSION_STR, "$abinumstr")
  AC_DEFINE_UNQUOTED(__ABI_TAG_VERSION, $abinum)
else
  AC_MSG_ERROR([*** The available kernel headers are older than the requested
*** compatible kernel version])
fi

if test -n "$sysheaders"; then
  CPPFLAGS=$OLD_CPPFLAGS
fi

if test "$prefix" = "/usr/local" -o "$prefix" = "/usr/local/" -o "$prefix" = "NONE"; then
  if test $enable_sanity = yes; then
    echo "\
*** On GNU/Linux systems the GNU C Library should not be installed into
*** /usr/local since this might make your system totally unusable.
*** We strongly advise to use a different prefix.  For details read the FAQ.
*** If you really mean to do this, run configure again using the extra
*** parameter \`--disable-sanity-checks'."
    exit 1
  else
    echo "\
*** WARNING: Do you really want to install the GNU C Library into /usr/local?
*** This might make your system totally unusable, for details read the FAQ."
  fi
fi


# One Linux we use ldconfig.
use_ldconfig=yes

if test $host = $build; then
  # If $prefix/include/{net,scsi} are symlinks, make install will
  # clobber what they're linked to (probably a kernel tree).
  # test -L ought to work on all Linux boxes.
  if test "x$prefix" != xNONE; then
    ac_prefix=$prefix
  else
    ac_prefix=$ac_default_prefix
  fi
  AC_MSG_CHECKING([for symlinks in ${ac_prefix}/include])
  ac_message=
  if test -L ${ac_prefix}/include/net; then
    ac_message="$ac_message
   ${ac_prefix}/include/net is a symlink"
  fi
  if test -L ${ac_prefix}/include/scsi; then
    ac_message="$ac_message
   ${ac_prefix}/include/scsi is a symlink"
  fi
  if test -n "$ac_message"; then
    AC_MSG_ERROR([$ac_message
'make install' will destroy the target of the link(s).
Delete the links and re-run configure, or better still, move the entire
${ac_prefix}/include directory out of the way.])
  else
    AC_MSG_RESULT(ok)
  fi
fi

# The atexit/quick_exit handler list is stored in the RELRO segment and its
# protection is toggled one runtime page at a time (stdlib/cxa_atexit.c).
# Rounding a block's extent up to a page must not reach past the RELRO
# segment into the writable data segment; that holds only if the linker pads
# the RELRO segment so that its end is aligned to the largest page the binary
# may run under.  EXEC_PAGESIZE from <asm/param.h> is that maximum for the
# target (e.g. 65536 on aarch64); the RELRO end alignment reflects the linker
# common-page-size.  A common-page-size smaller than EXEC_PAGESIZE also leaves
# RELRO under-protected by the dynamic loader on a kernel using the larger
# page, so reject such a toolchain here.
AC_CACHE_CHECK([whether $CC links a RELRO segment ending on a max-page boundary],
	       libc_cv_relro_page_aligned, [dnl
libc_cv_relro_page_aligned=unknown
cat > conftest.c <<EOF
#include <asm/param.h>
char page_probe@<:@EXEC_PAGESIZE@:>@;
EOF
libc_relro_max_page=
if ${CC-cc} $CFLAGS $CPPFLAGS -c conftest.c -o conftest.o \
     1>&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD; then
  libc_relro_max_page=`LC_ALL=C $READELF -sW conftest.o 2>/dev/null \
    | awk '$NF == "page_probe" { print $3; exit }'`
fi
cat > conftest.c <<EOF
extern int __libc_relro_probe_ext;
int *const __libc_relro_probe = &__libc_relro_probe_ext;
EOF
libc_relro_end=
if ${CC-cc} $CFLAGS $CPPFLAGS $LDFLAGS $no_ssp -shared -fPIC \
     -Wl,-z,relro -o conftest.so conftest.c \
     1>&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD; then
  set -- `LC_ALL=C $READELF -lW conftest.so 2>/dev/null \
    | awk '$1 == "GNU_RELRO" { print $3, $6; exit }'`
  if test $# -eq 2; then
    libc_relro_end=$(( $1 + $2 ))
  fi
fi
if test -n "$libc_relro_max_page" && test -n "$libc_relro_end"; then
  if test $(( libc_relro_end % libc_relro_max_page )) -eq 0; then
    libc_cv_relro_page_aligned=yes
  else
    libc_cv_relro_page_aligned=no
  fi
fi
rm -f conftest*])
case $libc_cv_relro_page_aligned in
  no)
    AC_MSG_ERROR([the linker pads the RELRO segment to a smaller page than the
maximum runtime page size (EXEC_PAGESIZE) of this target, so RELRO would be
under-protected at run time.  Use binutils 2.39 or newer, or link with
-Wl,-z,common-page-size=<EXEC_PAGESIZE>.]) ;;
  unknown)
    AC_MSG_WARN([could not determine the RELRO segment page alignment; assuming it matches the maximum runtime page size]) ;;
esac

# We have inlined syscalls.
AC_DEFINE(HAVE_INLINED_SYSCALLS)
