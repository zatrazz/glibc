/* Test the NLSPATH search of the gettext family of functions.
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

/* The message catalogs this test uses all translate a given msgid to
   "<marker>:<msgid>", where <marker> names the catalog.  Which catalog a
   lookup ended up in is therefore visible in the translation.

   Every case that expects a translation to be found uses a msgid of its
   own, so that the cache of known translations, which is not aware of
   NLSPATH, cannot serve one case from the result of an earlier one.  */

#include <libintl.h>
#include <locale.h>
#include <stdlib.h>

#include <support/check.h>
#include <support/support.h>
#include <support/xunistd.h>

#define DIR OBJPFX "nlspathdir/"

/* The locale all lookups but the last one are made in.  Its parts are what
   the %L, %l, %t, and %c conversion specifications expand to.  */
#define LOCALE "de_DE.UTF-8"

/* Look up MSGID with NLSPATH set to PATH, or unset if PATH is NULL, and
   check that the result is EXPECTED.  */
static void
check (const char *path, const char *msgid, const char *expected)
{
  if (path == NULL)
    unsetenv ("NLSPATH");
  else
    setenv ("NLSPATH", path, 1);

  TEST_COMPARE_STRING (gettext (msgid), expected);
}

static int
do_test (void)
{
  unsetenv ("LANGUAGE");
  unsetenv ("OUTPUT_CHARSET");
  xsetlocale (LC_ALL, LOCALE);

  textdomain ("nlspath");
  bindtextdomain ("nlspath", DIR "bound");

  /* Without NLSPATH the catalog is the one below the bound directory.  */
  check (NULL, "by-fallback", "bound:by-fallback");

  /* An empty NLSPATH is as good as an unset one.  */
  check ("", "by-empty-var", "bound:by-empty-var");

  /* Each conversion specification expands to the documented value.  */
  check (DIR "byname/%N.mo", "by-name", "byname:by-name");
  check (DIR "bylocale/%L.mo", "by-locale", "bylocale:by-locale");
  check (DIR "byparts/%l/%t/%c.mo", "by-parts", "byparts:by-parts");
  check (DIR "pct%%/%N.mo", "by-percent", "bypercent:by-percent");

  /* A leading, a trailing, and two adjacent colons all stand for a "%N"
     template, which here names a catalog relative to the current working
     directory.  */
  xchdir (DIR "bycwd");
  check (":", "by-empty", "bycwd:by-empty");
  xchdir (OBJPFX);

  /* Templates are tried in turn, skipping the ones naming a file that does
     not exist and the ones naming a file that is not a catalog.  */
  check (DIR "nosuch/%N.mo:" DIR "notacatalog:" DIR "first/%N.mo:"
	 DIR "byname/%N.mo", "by-order", "first:by-order");

  /* A template using an undefined conversion specification is ignored as a
     whole, and so is one ending in a lone '%'.  Both templates below would
     name the "byname" catalog if the stray conversion specification were
     merely dropped, so the "first" catalog is what tells the two apart.  */
  check (DIR "byname/%Q%N.mo:" DIR "first/%N.mo", "by-bad-spec",
	 "first:by-bad-spec");
  check (DIR "byname/%N.mo%:" DIR "first/%N.mo", "by-trailing-pct",
	 "first:by-trailing-pct");

  /* The NLSPATH search comes before the search below the bound directory,
     even though both catalogs know the msgid.  */
  check (DIR "byname/%N.mo", "by-precedence", "byname:by-precedence");

  /* LANGUAGE has no say in the NLSPATH search: %L expands to the locale
     name of the category, not to a locale name from LANGUAGE.  */
  setenv ("LANGUAGE", "fr_FR", 1);
  check (DIR "bylocale/%L.mo", "by-language", "bylocale:by-language");
  unsetenv ("LANGUAGE");

  /* No translation at all takes place in the C locale, so NLSPATH is not
     even looked at there.  */
  xsetlocale (LC_ALL, "C");
  check (DIR "byname/%N.mo", "by-c-locale", "by-c-locale");

  return 0;
}

#include <support/test-driver.c>
