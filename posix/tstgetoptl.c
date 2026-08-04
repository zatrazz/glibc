/* Check that getopt uses translated option names.  */
/* Copyright (C) 2026 Free Software Foundation, Inc.
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#include <locale.h>
#include <array_length.h>
#include <support/support.h>
#include <support/check.h>

/* This tests that --colour is accepted as a translation of --color.
   This echoes tstgetopt.c, where --colour was an option name alias
   for --color, so it had to be listed twice.  */

/* This uses the en_GB locale so that colour means color.  As a
   special case, we also check that non-translated options have
   precedence over translated options, by translating "optional" as
   "required".  We also check that getopt only matches translations
   for actual options, by having the user pass --flavour (which is a
   known translation of flavor) without the program recognizing a
   --flavor option.  */

static void
prepare_localedir (void)
{
  unsetenv ("LANGUAGE");
  xsetlocale (LC_MESSAGES, "en_GB.UTF-8");
  TEST_VERIFY_EXIT (bindtextdomain ("tstgetoptl", OBJPFX "domaindir") != NULL);
  TEST_VERIFY_EXIT (textdomain ("tstgetoptl") != NULL);
  /* Check that the catalog is OK: */
  TEST_COMPARE_STRING (gettext ("color"), "colour");
  TEST_COMPARE_STRING (gettext ("flavor"), "flavour");
}

static char **
prepare_argv (int *argc)
{
  static char *argv[] =
    {
      (char *) "tstgetoptl", (char *) "--required", (char *) "foobar",
      (char *) "--optional=bazbug", (char *) "--col", (char *) "--color",
      (char *) "--colour", (char *) "--flavour", NULL
    };
  *argc = array_length (argv) - 1;
  return argv;
}

static void
do_my_test (void)
{
  int argc;
  char **argv = prepare_argv (&argc);
  static const struct option options[] =
    {
      {"required", required_argument, NULL, 'r'},
      {"optional", optional_argument, NULL, 'o'},
      {"color",	   no_argument,	      NULL, 'C'},
      /* Now colour is handled as a translation of color.  */
      /* Note that there’s no "--flavor" option, so the "flavor" ->
	 "flavour" translation is useless.  */
      {NULL, 0, NULL, 0 }
    };

  /* This tests the same arguments as tstgetopt.c.  */

  int Cflag = 0;
  int index;
  int c;
  bool found_flavor = false;

  optind = 0;
  fputs ("Reminder that --flavor is not an option of the program.\n", stderr);
  while ((c = getopt_long (argc, argv, "", options, NULL)) >= 0)
    switch (c)
      {
      case 'C':
	++Cflag;
	break;
      case '?':
	TEST_VERIFY (!found_flavor);
	found_flavor = true;
	break;
      default:
	/* This should not happen.  */
	support_record_failure_reset ();
	return;

      case 'r':
	printf ("--required %s\n", optarg);
	TEST_COMPARE_STRING (optarg, "foobar");
	break;
      case 'o':
	printf ("--optional %s\n", optarg);
	if (optarg != NULL)
	  TEST_COMPARE_STRING (optarg, "bazbug");
	break;
      }

  TEST_VERIFY (found_flavor);

  printf ("Cflags = %d\n", Cflag);

  TEST_COMPARE (Cflag, 3);

  for (index = optind; index < argc; index++)
    printf ("Non-option argument %s\n", argv[index]);

  TEST_COMPARE (optind, argc);
}

int
do_test (void)
{
  prepare_localedir ();
  do_my_test ();
  return 0;
}

#define TEST_FUNCTION do_test
#include <support/test-driver.c>
