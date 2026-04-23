/* getopt_long and getopt_long_only entry points for GNU getopt.
   Copyright (C) 1987-2026 Free Software Foundation, Inc.
   This file is part of the GNU C Library and is also part of gnulib.
   Patches to this file should be submitted to both projects.

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

#ifndef _LIBC
# include <config.h>
# include "gettext.h"
#else
# include <libintl.h>
#endif

#include "getopt.h"
#include "getopt_int.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Callers store an optional context to enable option name
   translation.  The argument is allocated.  */

char *optctxt = NULL;

/* Callers store the textdomain in which the option names are to be
   looked up.  */

char *opttextdomain = NULL;

/* FIXME: use pgettext_expr.  */
static char *
do_translate (const char *domain, const char *context, const char *msgid,
	      char **allocated)
{
  char *full_msgid;
  const char *translated = msgid;
  int output_length = 0;

  *allocated = NULL;
  if (context != NULL)
    {
      output_length = __asprintf (&full_msgid, "%s\004%s", context, msgid);
      *allocated = full_msgid;
      if (output_length >= 0)
	{
	  translated = __dcgettext (domain, full_msgid, LC_MESSAGES);
	  if (strcmp (translated, full_msgid) == 0)
	    {
	      /* No translation for this context and message, so drop
		 the context + ^D prefix.  */
	      translated = msgid;
	    }
	}
      /* Otherwise, if memory allocation failed, then we won’t accept
	 translations.  translated remains an alias to msgid.  */
    }
  else
    translated = msgid;
  return (char *) translated;
}

int
getopt_long (int argc, char *__getopt_argv_const *argv, const char *options,
	     const struct option *long_options, int *opt_index)
{
  return _getopt_internal (argc, (char **) argv, options, long_options,
			   opt_index, 0, 0, do_translate,
			   optctxt, opttextdomain);
}

int
_getopt_long_r (int argc, char **argv, const char *options,
		const struct option *long_options, int *opt_index,
		struct _getopt_data *d)
{
  return _getopt_internal_r (argc, argv, options, long_options, opt_index,
			     0, d, 0, do_translate);
}

/* Like getopt_long, but '-' as well as '--' can indicate a long option.
   If an option that starts with '-' (not '--') doesn't match a long option,
   but does match a short option, it is parsed as a short option
   instead.  */

int
getopt_long_only (int argc, char *__getopt_argv_const *argv,
		  const char *options,
		  const struct option *long_options, int *opt_index)
{
  return _getopt_internal (argc, (char **) argv, options, long_options,
			   opt_index, 1, 0, do_translate,
			   optctxt, opttextdomain);
}

int
_getopt_long_only_r (int argc, char **argv, const char *options,
		     const struct option *long_options, int *opt_index,
		     struct _getopt_data *d)
{
  return _getopt_internal_r (argc, argv, options, long_options, opt_index,
			     1, d, 0, do_translate);
}

static void
disable_translations (void)
{
  free (optctxt);
  free (opttextdomain);
  optctxt = NULL;
  opttextdomain = NULL;
}

int
getopt_long_enable_translations (const char *msgctxt, const char *textdomain)
{
  disable_translations ();
  if (msgctxt != NULL)
    {
      optctxt = __strdup (msgctxt);
      if (textdomain)
	opttextdomain = __strdup (textdomain);
      if (optctxt == NULL
	  || (textdomain != NULL && opttextdomain == NULL))
	{
	  /* strdup failure */
	  disable_translations ();
	  return -1;
	}
    }
  return 0;
}

void
getopt_long_disable_translations (void)
{
  disable_translations ();
}


#ifdef TEST

#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  int c;
  int digit_optind = 0;

  while (1)
    {
      int this_option_optind = optind ? optind : 1;
      int option_index = 0;
      static const struct option long_options[] =
      {
	{"add", 1, 0, 0},
	{"append", 0, 0, 0},
	{"delete", 1, 0, 0},
	{"verbose", 0, 0, 0},
	{"create", 0, 0, 0},
	{"file", 1, 0, 0},
	{0, 0, 0, 0}
      };

      c = getopt_long (argc, argv, "abc:d:0123456789",
		       long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  printf ("option %s", long_options[option_index].name);
	  if (optarg)
	    printf (" with arg %s", optarg);
	  printf ("\n");
	  break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  if (digit_optind != 0 && digit_optind != this_option_optind)
	    printf ("digits occur in two different argv-elements.\n");
	  digit_optind = this_option_optind;
	  printf ("option %c\n", c);
	  break;

	case 'a':
	  printf ("option a\n");
	  break;

	case 'b':
	  printf ("option b\n");
	  break;

	case 'c':
	  printf ("option c with value '%s'\n", optarg);
	  break;

	case 'd':
	  printf ("option d with value '%s'\n", optarg);
	  break;

	case '?':
	  break;

	default:
	  printf ("?? getopt returned character code 0%o ??\n", c);
	}
    }

  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
	printf ("%s ", argv[optind++]);
      printf ("\n");
    }

  exit (0);
}

#endif /* TEST */
