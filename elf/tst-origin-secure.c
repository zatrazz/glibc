/* Test that AT_SECURE $ORIGIN rpath entries are looked up using the
   normalized (trusted) path, not the raw expansion (bug 34360).

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

/* For a SUID/SGID program the loader only honors $ORIGIN in DT_RPATH when
   the *normalized* expansion is rooted in a trusted directory.  The bug is
   that the check normalizes "../" lexically but the lookup then opens the
   raw, un-normalized string; those two disagree as soon as a path
   component is a symbolic link.

   This test builds the victim with the rpath

     $ORIGIN/sub/../../../../..SLIBDIR/tst-origin-secure

   and runs it from ORIGIN = /tmp/tst-origin-secure/a/b.  Lexically the five
   "../" pop /tmp/tst-origin-secure/a/b/sub back to "/", so the entry
   normalizes to the trusted SLIBDIR/tst-origin-secure.  But "sub" is a
   symlink pointing five levels deep under an attacker directory, so opening
   the raw string makes the kernel resolve the "../" through the symlink and
   land in /tmp/tst-origin-secure/x1 SLIBDIR/tst-origin-secure instead.

   A trusted copy of the module (returning 1) is installed in
   SLIBDIR/tst-origin-secure; an attacker copy (returning 2) is placed at
   the symlink-diverted location.  The trusted subdirectory is rooted under
   SLIBDIR -- so it passes the trusted-path check -- but is not itself a
   default loader search directory, so loading the trusted copy proves the
   normalized rpath entry was actually honored rather than the module being
   found on the default search path.

   The victim reports, in its exit status, both which module it loaded and
   whether it ran in AT_SECURE mode (see tst-origin-secure-victim.c), so a
   real bypass (status 3) is told apart from a run that was not secure
   (status 0 or 1).

   Secure mode is forced with glibc.rtld.enable_secure=1 so that no real
   SUID/SGID binary (or root) is required.  This is a test-container test so
   that SLIBDIR is a real, writable, trusted directory and the paths have a
   fixed depth.  */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <support/check.h>
#include <support/support.h>
#include <support/xunistd.h>

#define BASE   "/tmp/tst-origin-secure"
#define SONAME "libtst-origin-secure-mod.so"
/* Subdirectory of the trusted SLIBDIR that the rpath normalizes to.  It is
   trusted (rooted under SLIBDIR) but not a default search directory.  */
#define SUBDIR "tst-origin-secure"

/* Run the victim at BASE/a/b/victim with the given (NULL-terminated)
   environment and return its exit status, or -1 if it did not exit
   normally.  The status encodes the loaded module in bit 0 (set for the
   attacker copy) and AT_SECURE in bit 1.  */
static int
run_victim (char *const *envp)
{
  const char *victim = BASE "/a/b/victim";
  char *const argv[] = { (char *) victim, NULL };

  pid_t pid = xfork ();
  if (pid == 0)
    {
      execve (victim, argv, (char *const *) envp);
      _exit (127);
    }

  int status;
  xwaitpid (pid, &status, 0);
  return WIFEXITED (status) ? WEXITSTATUS (status) : -1;
}

static int
do_test (void)
{
  const char *slibdir = support_slibdir_prefix;         /* e.g. "/lib64".  */
  const char *objelf = support_objdir_root;             /* build root.  */

  /* Source artifacts built in the elf/ object directory.  */
  char *good_src = xasprintf ("%s/elf/libtst-origin-secure-mod.so", objelf);
  char *evil_src = xasprintf ("%s/elf/tst-origin-secure-evilmod.so", objelf);
  char *victim_src = xasprintf ("%s/elf/tst-origin-secure-victim", objelf);

  /* $ORIGIN of the victim, the symlink, and its (deep) target.  The target
     is five levels below BASE/evil so that the five "../" in the rpath,
     applied after the symlink, resolve to BASE/evil.  */
  xmkdirp (BASE "/a/b", 0755);
  xmkdirp (BASE "/x1/x2/x3/x4/x5/x6", 0755);

  /* Where the trusted copy lives (reached only via the normalized rpath,
     SLIBDIR/SUBDIR) ...  */
  char *good_dir = xasprintf ("%s/%s", slibdir, SUBDIR);
  char *good_dst = xasprintf ("%s/%s", good_dir, SONAME);
  xmkdirp (good_dir, 0755);
  /* ... and where the raw, symlink-diverted lookup lands.  */
  char *evil_dir = xasprintf ("%s/x1%s/%s", BASE, slibdir, SUBDIR);
  char *evil_dst = xasprintf ("%s/%s", evil_dir, SONAME);
  xmkdirp (evil_dir, 0755);

  support_copy_file (good_src, good_dst);
  support_copy_file (evil_src, evil_dst);
  support_copy_file (victim_src, BASE "/a/b/victim");
  xchmod (BASE "/a/b/victim", 0755);

  /* Tolerate leftovers from an earlier run at this fixed path.  */
  unlink (BASE "/a/b/sub");
  xsymlink (BASE "/x1/x2/x3/x4/x5/x6", BASE "/a/b/sub");

  /* The five "../" in the rpath assume the victim sits at a fixed lexical
     depth and that $ORIGIN -- which the loader derives from the real,
     symlink-resolved path of the executable -- matches that depth.  If a
     component of BASE is itself a symlink (for example /tmp on some
     systems), $ORIGIN resolves to a different depth and the layout cannot
     reproduce the divergence; treat that as unsupported rather than as a
     failure, so that the runs below can fail hard on a real regression.  */
  {
    char *real = realpath (BASE "/a/b/victim", NULL);
    if (real == NULL || strcmp (real, BASE "/a/b/victim") != 0)
      FAIL_UNSUPPORTED ("victim real path is %s, expected %s: a symlinked "
			"path component prevents the $ORIGIN layout from "
			"reproducing the divergence",
			real == NULL ? "(null)" : real, BASE "/a/b/victim");
    free (real);
  }

  /* Control run: in normal mode $ORIGIN is honored without the trusted
     check, so the raw rpath resolves through "sub" and the attacker copy is
     loaded.  The depth precondition above is verified, so this must
     reproduce: the victim reports "attacker copy, not secure" (status 1).  */
  {
    char *const env[] = { NULL };
    TEST_COMPARE (run_victim (env), 1);
  }

  /* Secure run: force AT_SECURE.  A fixed loader normalizes the rpath to the
     trusted SLIBDIR/SUBDIR and loads the trusted copy; a loader with the bug
     opens the raw path, resolves "sub", and loads the attacker copy.  */
  {
    char *const env[] = { (char *) "GLIBC_TUNABLES=glibc.rtld.enable_secure=1",
			  NULL };
    int rc = run_victim (env);
    switch (rc)
      {
      case 2:
	/* AT_SECURE, trusted copy loaded via the normalized rpath: fixed.  */
	break;
      case 3:
	/* AT_SECURE, attacker copy loaded: the raw rpath was opened.  */
	FAIL_EXIT1 ("AT_SECURE loader resolved the un-normalized rpath "
		    "through the attacker symlink (bug 34360)");
      case 0:
      case 1:
	/* AT_SECURE bit clear: the tunable did not engage secure mode, so
	   this run says nothing about the trusted-path handling.  */
	FAIL_UNSUPPORTED ("glibc.rtld.enable_secure=1 did not enable "
			  "AT_SECURE mode (victim status %d)", rc);
      default:
	/* Neither copy loaded (e.g. status 127): since the trusted copy is
	   reachable only through the normalized rpath, this means the rpath
	   entry was not honored at all.  */
	FAIL_EXIT1 ("secure run did not load the module via the normalized "
		    "rpath (victim status %d)", rc);
      }
  }

  free (good_src);
  free (evil_src);
  free (victim_src);
  free (good_dir);
  free (good_dst);
  free (evil_dir);
  free (evil_dst);
  return 0;
}

#include <support/test-driver.c>
