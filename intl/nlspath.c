/* NLSPATH handling for the gettext family of functions.
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

/* POSIX.1-2024 makes an NLSPATH search the first of the three steps that
   locate the messages object of a gettext call, before the LANGUAGE search
   and before the search relative to the directory bound to the text domain.
   NLSPATH holds a colon separated list of templates, each of which expands
   to the whole file name of a messages object (XBD 8.2 Internationalization
   Variables.  catopen uses the same variable, with the same conversion
   specifications but with a different defaulti.  */

#include <stdlib.h>
#include <string.h>

#include <scratch_buffer.h>

#include "gettextP.h"
#include "loadinfo.h"

/* A value a conversion specification expands to.  It is not necessarily NUL
   terminated, since the pieces of a locale name are substrings of it.  */
struct nlspath_value
{
  const char *ptr;
  size_t len;
};

struct nlspath_values
{
  struct nlspath_value name;		/* %N */
  struct nlspath_value locale;		/* %L */
  struct nlspath_value language;	/* %l */
  struct nlspath_value territory;	/* %t */
  struct nlspath_value codeset;		/* %c */
};

/* Set VALUES up for the text domain NAME and the locale name LOCALE.  A
   locale name has the form

     language[_territory][.codeset][@modifier]

   where everything but the language part may be missing.  The separators
   are not part of the %t and %c expansions, and no conversion
   specification expands to the modifier.  */
static void
set_nlspath_values (struct nlspath_values *values, const char *name,
		    const char *locale)
{
  const char *cp;

  values->name.ptr = name;
  values->name.len = strlen (name);

  values->locale.ptr = locale;
  values->locale.len = strlen (locale);

  values->territory.ptr = "";
  values->territory.len = 0;
  values->codeset.ptr = "";
  values->codeset.len = 0;

  cp = locale;
  while (*cp != '\0' && *cp != '_' && *cp != '.' && *cp != '@')
    ++cp;
  values->language.ptr = locale;
  values->language.len = cp - locale;

  if (*cp == '_')
    {
      const char *start = ++cp;
      while (*cp != '\0' && *cp != '.' && *cp != '@')
	++cp;
      values->territory.ptr = start;
      values->territory.len = cp - start;
    }

  if (*cp == '.')
    {
      const char *start = ++cp;
      while (*cp != '\0' && *cp != '@')
	++cp;
      values->codeset.ptr = start;
      values->codeset.len = cp - start;
    }
}

/* Expand the NLSPATH template running from TMPL up to, but not including,
   TMPL_END, substituting each conversion specification by the matching
   member of VALUES.  If BUF is not NULL the expansion is written to it,
   followed by a NUL byte.  Return the length of the expansion, not counting
   that NUL byte, or (size_t) -1 if the template uses a conversion
   specification that is not defined, in which case the whole template has
   to be ignored.  */
static size_t
expand_template (const char *tmpl, const char *tmpl_end,
		 const struct nlspath_values *values, char *buf)
{
  size_t len = 0;

  while (tmpl < tmpl_end)
    {
      const struct nlspath_value *value;
      static const struct nlspath_value percent = { "%", 1 };

      if (*tmpl != '%')
	{
	  if (buf != NULL)
	    buf[len] = *tmpl;
	  ++len;
	  ++tmpl;
	  continue;
	}

      /* A '%' at the very end of a template has no conversion specifier to
	 go with it; treat it like an undefined one.  */
      if (++tmpl == tmpl_end)
	return -1;

      switch (*tmpl++)
	{
	case 'N':
	  value = &values->name;
	  break;
	case 'L':
	  value = &values->locale;
	  break;
	case 'l':
	  value = &values->language;
	  break;
	case 't':
	  value = &values->territory;
	  break;
	case 'c':
	  value = &values->codeset;
	  break;
	case '%':
	  value = &percent;
	  break;
	default:
	  return -1;
	}

      if (buf != NULL)
	memcpy (buf + len, value->ptr, value->len);
      len += value->len;
    }

  if (buf != NULL)
    buf[len] = '\0';

  return len;
}

/* Look up MSGID in the messages objects named by the templates of NLSPATH,
   expanded for the text domain DOMAINNAME and the locale name LOCALENAME,
   and converted to the output encoding given by DOMAINBINDING.  Templates
   are tried in turn until one of them yields a messages object containing
   MSGID.

   On success return the translation, store its length in *LENGTHP and the
   messages object it came from in *DOMAINP.  Return NULL if no template
   led to a translation, or (char *) -1 if a resource problem kept one from
   being converted.  */
char *
_nl_find_msg_nlspath (const char *nlspath, const char *domainname,
		      const char *localename, struct binding *domainbinding,
		      const char *msgid, size_t *lengthp,
		      struct loaded_l10nfile **domainp)
{
  struct nlspath_values values;
  struct scratch_buffer path;
  char *retval = NULL;
  const char *runp = nlspath;

  set_nlspath_values (&values, domainname, localename);
  scratch_buffer_init (&path);

  while (1)
    {
      const char *end = __strchrnul (runp, ':');
      const char *tmpl = runp;
      const char *tmpl_end = end;
      struct loaded_l10nfile *domain;
      size_t len;

      /* A leading, a trailing, and two adjacent colons are all equivalent
	 to a "%N" template.  */
      if (tmpl == tmpl_end)
	{
	  tmpl = "%N";
	  tmpl_end = tmpl + 2;
	}

      len = expand_template (tmpl, tmpl_end, &values, NULL);

      /* Skip templates using an undefined conversion specification, and
	 those that expand to nothing at all.  */
      if (len == (size_t) -1 || len == 0)
	goto next;

      if (!scratch_buffer_set_array_size (&path, len + 1, 1))
	{
	  retval = (char *) -1;
	  break;
	}
      expand_template (tmpl, tmpl_end, &values, path.data);

      /* Files that do not hold a valid messages object are ignored, as are
	 valid ones that do not know MSGID.  */
      domain = _nl_find_domain_file (path.data, domainbinding);
      if (domain == NULL)
	goto next;

      retval = _nl_find_msg (domain, domainbinding, msgid, 1, lengthp);
      if (retval == (char *) -1)
	break;
      if (retval != NULL)
	{
	  *domainp = domain;
	  break;
	}

    next:
      if (*end == '\0')
	break;
      runp = end + 1;
    }

  scratch_buffer_free (&path);

  return retval;
}
