/* Copyright (C) 1991-2026 Free Software Foundation, Inc.
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

#ifndef _LIBC
# include <libc-config.h>
#endif

/* Enable GNU extensions in fnmatch.h.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE    1
#endif

#include <fnmatch.h>

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

/* The collation-sequence data used by collating elements, equivalence
   classes and ranges is only reachable inside glibc.  Outside glibc these
   bracket features degrade to plain character comparisons, matching the
   historical gnulib behaviour.  */
#ifdef _LIBC
# include <shlib-compat.h>
# include <locale/coll-lookup.h>
# include <locale/localeinfo.h>
# include <locale/weight.h>
/* Rename the wide-char findidx so it doesn't clash with the byte version
   pulled in above.  */
# define findidx findidxwc
# include <locale/weightwc.h>
# undef findidx
#else
# define __mbrtowc   mbrtowc
# define __towlower  towlower
# define __iswctype  iswctype
# define __wctype    wctype
# define __mempcpy   mempcpy
# define __fnmatch   fnmatch
#endif

/* We often have to test for FNM_FILE_NAME and FNM_PERIOD being both set.  */
#define NO_LEADING_PERIOD(flags) \
  ((flags & (FNM_FILE_NAME | FNM_PERIOD)) == (FNM_FILE_NAME | FNM_PERIOD))

/* Maximum character class name length supported by the wide character
   class support of glibc.  */
#define CHAR_CLASS_MAX_LENGTH CHARCLASS_NAME_MAX

/* Cached POSIXLY_CORRECT environment value.  Tri-state:
   0  not yet checked
   1  POSIXLY_CORRECT is set
  -1  POSIXLY_CORRECT is not set.  */
static int posixly_correct;

/* Pair of (pattern, string) positions used by the wildcard backtracking
   loop in internal_fnmatch.  */
struct fnm_ends
{
  const char *pattern;
  const char *string;
  bool no_leading_period;
};

/* Result of decoding one logical character.  */
struct fnm_char
{
  wint_t wc;             /* The character value (possibly a marker).  */
  unsigned char bytes;   /* Number of input bytes that this character covers.  */
};

/* Special "invalid byte" marker base.  Adding the byte value to this base
   yields a wint_t value that cannot collide with any valid Unicode code
   point (which goes up to 0x10FFFF) and therefore only matches itself.

   This relies on glibc's internal wide-character representation being
   UCS-4 / Unicode, whose code points never reach FNM_BAD_BYTE_BASE.  A
   marker is FNM_BAD_BYTE_BASE + c with c in 0x80..0xFF, so the whole marker
   range must also stay strictly below WEOF.  */
#define FNM_BAD_BYTE_BASE 0xE0000000U
_Static_assert (FNM_BAD_BYTE_BASE > 0x10FFFFU,
                "FNM_BAD_BYTE_BASE must not collide with valid code points");
_Static_assert (FNM_BAD_BYTE_BASE + 0xFFU < (wint_t) WEOF,
                "FNM_BAD_BYTE_BASE range must stay below WEOF");

/* Peek the next logical character from P (up to END), using PS state.
   Returns the character along with the byte length it covers, and 0 at EOF.
   This does not advance any pointer or mutate PS.  */
static inline struct fnm_char
fnm_peek (const char *p, const char *end, mbstate_t *ps, int mb_cur_max)
{
  if (p >= end)
    return (struct fnm_char) { .wc = WEOF, .bytes = 0 };

  unsigned char c = (unsigned char) *p;
  if (mb_cur_max == 1 || c < 0x80)
    return (struct fnm_char) { .wc = c, .bytes = 1 };

  wchar_t wc;
  mbstate_t local = *ps;
  size_t n = __mbrtowc (&wc, p, end - p, &local);
  if (n == 0)
    /* Embedded NUL.  We only get here with explicit end pointer.  */
    return (struct fnm_char) { .wc = L'\0', .bytes = 1 };

  if (n == (size_t) -1 || n == (size_t) -2)
     /* Invalid or incomplete sequence.  Treat the first byte as a private
	marker that won't match valid Unicode characters.  */
    return (struct fnm_char) { .wc = FNM_BAD_BYTE_BASE + c, .bytes = 1 };

  return (struct fnm_char) { .wc = wc, .bytes = n };
}

/* Advance *PP past CH, a logical character previously peeked at *PP, and
   update the multibyte state PS to match.  CH must be different than 0.  */
static inline void
fnm_consume (const char **pp, struct fnm_char ch, mbstate_t *ps,
             int mb_cur_max)
{
  *pp += ch.bytes;
  if (mb_cur_max != 1 && ch.wc >= 0x80 && ch.wc < FNM_BAD_BYTE_BASE)
    {
      /* Re-run the conversion to advance the real mbstate.  */
      wchar_t wc;
      __mbrtowc (&wc, *pp - ch.bytes, ch.bytes, ps);
    }
  else if (ch.wc >= FNM_BAD_BYTE_BASE)
    memset (ps, '\0', sizeof (*ps));
}

/* Decode and consume one logical character from *PP, updating both the
   pointer and the multibyte state PS.  */
static inline struct fnm_char
fnm_decode (const char **pp, const char *end, mbstate_t *ps, int mb_cur_max)
{
  struct fnm_char r = fnm_peek (*pp, end, ps, mb_cur_max);
  if (r.bytes != 0)
    fnm_consume (pp, r, ps, mb_cur_max);
  return r;
}

/* Fold case for a single character.  */
static inline wint_t
fnm_fold (wint_t c, int flags, int mb_cur_max)
{
  if (!(flags & FNM_CASEFOLD))
    return c;
  if (c < 0x80 || mb_cur_max == 1)
    return tolower ((unsigned char) c);
  if (c >= FNM_BAD_BYTE_BASE)
    return c;
  return __towlower (c);
}
#define FOLD(c) fnm_fold ((c), flags, mb_cur_max)

/* Read the next pattern character (literal position).  P is advanced.  */
static inline struct fnm_char
pat_next (const char **p, mbstate_t *ps, int mb_cur_max)
{
  /* Patterns are null-terminated; we treat any embedded NUL as end.
     Use a sentinel end pointer at the NUL.  */
  const char *end = *p;
  while (*end != '\0')
    ++end;
  return fnm_decode (p, end, ps, mb_cur_max);
}

static int internal_fnmatch (const char *pattern, const char *string,
                             const char *string_end, bool no_leading_period,
                             int flags, struct fnm_ends *ends,
                             int mb_cur_max);
static int ext_match (int opt, const char *pattern, const char *string,
                      const char *string_end, bool no_leading_period,
                      int flags, int mb_cur_max);
static const char *end_pattern (const char *patternp);

static int
internal_fnmatch (const char *pattern, const char *string,
                  const char *string_end, bool no_leading_period,
                  int flags, struct fnm_ends *ends, int mb_cur_max)
{
  const char *p = pattern;
  const char *n = string;
  mbstate_t pps, nps;
  memset (&pps, '\0', sizeof (pps));
  memset (&nps, '\0', sizeof (nps));

#ifdef _LIBC
  const unsigned char *collseqmb = (const unsigned char *)
    _NL_CURRENT (LC_COLLATE, _NL_COLLATE_COLLSEQMB);
  const char *collseqwc = (const char *)
    _NL_CURRENT (LC_COLLATE, _NL_COLLATE_COLLSEQWC);
#endif

  while (*p != '\0')
    {
      unsigned char pc = *p;
      bool new_no_leading_period = false;

      /* Pattern syntax characters are ASCII; check the raw byte.  */
      switch (pc)
        {
        case '?':
          ++p;
          if (__glibc_unlikely (flags & FNM_EXTMATCH) && *p == '(')
            {
              int res = ext_match ('?', p, n, string_end,
                                   no_leading_period, flags, mb_cur_max);
              if (res != -1)
                return res;
            }

          if (n == string_end)
            return FNM_NOMATCH;

          {
            struct fnm_char ch = fnm_peek (n, string_end, &nps, mb_cur_max);
            if (ch.wc == (wint_t) '/' && (flags & FNM_FILE_NAME))
              return FNM_NOMATCH;
            if (ch.wc == (wint_t) '.' && no_leading_period)
              return FNM_NOMATCH;
            fnm_consume (&n, ch, &nps, mb_cur_max);
          }
          break;

        case '\\':
          ++p;
          if (!(flags & FNM_NOESCAPE))
            {
              if (*p == '\0')
                /* Trailing \ loses.  */
                return FNM_NOMATCH;
              struct fnm_char pat = pat_next (&p, &pps, mb_cur_max);
              wint_t pwc = FOLD (pat.wc);
              if (n == string_end)
                return FNM_NOMATCH;
              struct fnm_char nc = fnm_decode (&n, string_end, &nps,
                                               mb_cur_max);
              if (FOLD (nc.wc) != pwc)
                return FNM_NOMATCH;
            }
          else
            {
              /* Compare the backslash literally.  */
              if (n == string_end)
                return FNM_NOMATCH;
              struct fnm_char nc = fnm_decode (&n, string_end, &nps,
                                               mb_cur_max);
              if (FOLD (nc.wc) != FOLD ((wint_t) '\\'))
                return FNM_NOMATCH;
            }
          break;

        case '*':
          ++p;
          if (__glibc_unlikely (flags & FNM_EXTMATCH) && *p == '(')
            {
              int res = ext_match ('*', p, n, string_end,
                                   no_leading_period, flags, mb_cur_max);
              if (res != -1)
                return res;
            }
          else if (ends != NULL)
            {
              ends->pattern = p - 1;
              ends->string = n;
              ends->no_leading_period = no_leading_period;
              return 0;
            }

          if (n != string_end)
            {
              struct fnm_char ch = fnm_peek (n, string_end, &nps, mb_cur_max);
              if (ch.wc == (wint_t) '.' && no_leading_period)
                return FNM_NOMATCH;
            }

          /* Collapse consecutive '?' and '*' wildcards.  */
          {
            unsigned char c;
            /* In the loop body, c is the just-read char and p is past it.
               After the loop, c is the first non-wildcard char and p is
               past it (so p - 1 points at c).  */
            for (c = *p++; c == '?' || c == '*'; c = *p++)
              {
                if (*p == '(' && (flags & FNM_EXTMATCH) != 0)
                  {
                    const char *endp = end_pattern (p - 1);
                    if (endp != p - 1)
                      {
                        /* This is a pattern.  Skip over it.  */
                        p = endp;
                        continue;
                      }
                  }

                if (c == '?')
                  {
                    /* A ? needs to match one character.  */
                    if (n == string_end)
                      return FNM_NOMATCH;
                    struct fnm_char ch = fnm_peek (n, string_end, &nps,
                                                   mb_cur_max);
                    if (ch.wc == (wint_t) '/'
                        && __glibc_unlikely (flags & FNM_FILE_NAME))
                      return FNM_NOMATCH;
                    fnm_consume (&n, ch, &nps, mb_cur_max);
                  }
              }

            if (c == '\0')
              {
                /* The wildcard(s) is/are the last element of the pattern.
                   If the name is a file name and contains another slash
                   this means it cannot match, unless the FNM_LEADING_DIR
                   flag is set.  */
                int result = (flags & FNM_FILE_NAME) == 0 ? 0 : FNM_NOMATCH;

                if (flags & FNM_FILE_NAME)
                  {
                    if (flags & FNM_LEADING_DIR)
                      result = 0;
                    else if (memchr (n, '/', string_end - n) == NULL)
                      result = 0;
                  }

                return result;
              }
            else
              {
                const char *endp;
                struct fnm_ends end;
                end.pattern = NULL;
                endp = memchr (n, (flags & FNM_FILE_NAME) ? '/' : '\0',
                               string_end - n);
                if (endp == NULL)
                  endp = string_end;

                if (c == '['
                    || (__glibc_unlikely (flags & FNM_EXTMATCH)
                        && (c == '@' || c == '+' || c == '!')
                        && *p == '('))
                  {
                    int flags2 = ((flags & FNM_FILE_NAME)
                                  ? flags : (flags & ~FNM_PERIOD));
                    const char *try_p = p - 1;
                    while (n <= endp)
                      {
                        if (internal_fnmatch (try_p, n, string_end,
                                              no_leading_period, flags2,
                                              &end, mb_cur_max) == 0)
                          goto found;
                        if (n == endp)
                          break;
                        struct fnm_char ch = fnm_peek (n, string_end, &nps,
                                                       mb_cur_max);
                        if (ch.bytes == 0)
                          break;
                        fnm_consume (&n, ch, &nps, mb_cur_max);
                        no_leading_period = false;
                      }
                  }
                else if (c == '/' && (flags & FNM_FILE_NAME))
                  {
                    while (n < string_end && *n != '/')
                      ++n;
                    if (n < string_end && *n == '/'
                        && (internal_fnmatch (p, n + 1, string_end,
                                              flags & FNM_PERIOD, flags,
                                              NULL, mb_cur_max) == 0))
                      return 0;
                  }
                else
                  {
                    int flags2 = ((flags & FNM_FILE_NAME)
                                  ? flags : (flags & ~FNM_PERIOD));

                    /* Determine the literal char in the pattern (folded).
                       The character starts at (p-1) for the byte we just
                       loaded; if it's an escape and FNM_NOESCAPE is off,
                       advance to the next char.  Otherwise it is c itself
                       (possibly multibyte starting at p-1).  */
                    wint_t target;
                    const char *try_p = p - 1;
                    if (c == '\\' && !(flags & FNM_NOESCAPE))
                      {
                        /* Decode the escaped character starting at p.  */
                        const char *escape_p = p;
                        mbstate_t ts;
                        memset (&ts, '\0', sizeof (ts));
                        struct fnm_char pat = pat_next (&escape_p, &ts,
                                                        mb_cur_max);
                        target = FOLD (pat.wc);
                      }
                    else
                      {
                        /* Decode multibyte literal starting at p-1, and
                           advance the real pattern pointer past it.  */
                        const char *lit_p = p - 1;
                        mbstate_t ts;
                        memset (&ts, '\0', sizeof (ts));
                        struct fnm_char pat = pat_next (&lit_p, &ts,
                                                        mb_cur_max);
                        target = FOLD (pat.wc);
                        p = lit_p;
                      }

                    while (n < endp)
                      {
                        struct fnm_char ch = fnm_peek (n, string_end, &nps,
                                                       mb_cur_max);
                        if (ch.bytes == 0)
                          break;
                        if (FOLD (ch.wc) == target
                            && internal_fnmatch (try_p, n, string_end,
                                                 no_leading_period, flags2,
                                                 &end, mb_cur_max) == 0)
                          {
                          found:
                            if (end.pattern == NULL)
                              return 0;
                            break;
                          }
                        fnm_consume (&n, ch, &nps, mb_cur_max);
                        no_leading_period = false;
                      }
                    if (end.pattern != NULL)
                      {
                        p = end.pattern;
                        n = end.string;
                        no_leading_period = end.no_leading_period;
                        memset (&nps, '\0', sizeof (nps));
                        memset (&pps, '\0', sizeof (pps));
                        continue;
                      }
                  }
              }
            /* If we come here no match is possible with the wildcard.  */
            return FNM_NOMATCH;
          }

        case '[':
          {
            /* Bracket expression.  */
            const char *p_init = p;
            const char *n_init = n;
            bool not;
            wint_t cold = 0;
            wint_t bracket_c = 0;
            wint_t fn;
            struct fnm_char nc;
            mbstate_t saved_nps;
#ifdef _LIBC
            bool is_seqval = false;
#endif

            ++p;

            if (posixly_correct == 0)
              posixly_correct = getenv ("POSIXLY_CORRECT") != NULL ? 1 : -1;

            if (n == string_end)
              return FNM_NOMATCH;

            saved_nps = nps;
            nc = fnm_peek (n, string_end, &nps, mb_cur_max);
            if (nc.wc == (wint_t) '.' && no_leading_period)
              return FNM_NOMATCH;
            if (nc.wc == (wint_t) '/' && (flags & FNM_FILE_NAME))
              return FNM_NOMATCH;

            not = (*p == '!' || (posixly_correct < 0 && *p == '^'));
            if (not)
              ++p;

            fn = FOLD (nc.wc);

            unsigned char pc2 = *p++;
            for (;;)
              {
                if (!(flags & FNM_NOESCAPE) && pc2 == '\\')
                  {
                    if (*p == '\0')
                      return FNM_NOMATCH;
                    /* Decode the escaped pattern char.  */
                    mbstate_t ts;
                    memset (&ts, '\0', sizeof (ts));
                    struct fnm_char pat = pat_next (&p, &ts, mb_cur_max);
                    cold = FOLD (pat.wc);
                    if ((unsigned char) *p != '-' || p[1] == ']'
                        || p[1] == '\0')
                      {
                        if (cold == fn)
                          goto matched;
                        pc2 = *p++;
                        if (pc2 == '-' && *p != ']')
                          goto handle_range;
                        if (pc2 == ']')
                          break;
                        continue;
                      }
                    /* Range.  */
                    pc2 = *p++;
                    goto handle_range;
                  }
                else if (pc2 == '[' && *p == ':')
                  {
                    /* Character class.  */
                    char str[CHAR_CLASS_MAX_LENGTH + 1];
                    size_t c1 = 0;
                    wctype_t wt;
                    const char *startp = p;

                    for (;;)
                      {
                        if (c1 == CHAR_CLASS_MAX_LENGTH)
                          return FNM_NOMATCH;
                        unsigned char cc = *++p;
                        if (cc == ':' && p[1] == ']')
                          {
                            p += 2;
                            break;
                          }
                        if (cc < 'a' || cc >= 'z')
                          {
                            /* Not a character class name.  */
                            p = startp;
                            pc2 = '[';
                            goto normal_bracket;
                          }
                        str[c1++] = cc;
                      }
                    str[c1] = '\0';

                    wt = __wctype (str);
                    if (wt == 0)
                      return FNM_NOMATCH;

                    if (mb_cur_max == 1 && nc.wc < 0x100)
                      {
#ifdef _LIBC
                        if (_ISCTYPE ((unsigned char) nc.wc, wt))
                          goto matched;
#else
                        if (iswctype (btowc ((unsigned char) nc.wc), wt))
                          goto matched;
#endif
                      }
                    else if (nc.wc < FNM_BAD_BYTE_BASE
                             && __iswctype (nc.wc, wt))
                      goto matched;

                    pc2 = *p++;
                  }
#ifdef _LIBC
                else if (pc2 == '[' && *p == '=')
                  {
                    /* Equivalence class.  */
                    uint32_t nrules =
                      _NL_CURRENT_WORD (LC_COLLATE, _NL_COLLATE_NRULES);
                    const char *startp = p;
                    /* Read the single character inside [=X=].  */
                    ++p;
                    if (*p == '\0')
                      {
                        p = startp;
                        pc2 = '[';
                        goto normal_bracket;
                      }
                    /* Decode this char (could be multibyte).  */
                    mbstate_t ts;
                    memset (&ts, '\0', sizeof (ts));
                    struct fnm_char eqch = pat_next (&p, &ts, mb_cur_max);
                    if (*p != '=' || p[1] != ']')
                      {
                        p = startp;
                        pc2 = '[';
                        goto normal_bracket;
                      }
                    p += 2;

                    if (nrules == 0)
                      {
                        if (FOLD (nc.wc) == FOLD (eqch.wc))
                          goto matched;
                      }
                    else if (mb_cur_max == 1
                             || (eqch.wc < 0x80 && nc.wc < 0x80))
                      {
                        const int32_t *table = (const int32_t *)
                          _NL_CURRENT (LC_COLLATE, _NL_COLLATE_TABLEMB);
                        const unsigned char *weights = (const unsigned char *)
                          _NL_CURRENT (LC_COLLATE, _NL_COLLATE_WEIGHTMB);
                        const unsigned char *extra = (const unsigned char *)
                          _NL_CURRENT (LC_COLLATE, _NL_COLLATE_EXTRAMB);
                        const int32_t *indirect = (const int32_t *)
                          _NL_CURRENT (LC_COLLATE, _NL_COLLATE_INDIRECTMB);
                        /* Two-byte buffer keeps findidx from reading past
                           the end of a single byte object.  */
                        unsigned char cb[2] = { eqch.wc, 0 };
                        const unsigned char *cp = cb;
                        int32_t idx = findidx (table, indirect, extra, &cp, 1);
                        if (idx != 0)
                          {
                            int len = weights[idx & 0xffffff];
                            const unsigned char *np
                              = (const unsigned char *) n;
                            int32_t idx2 = findidx (table, indirect,
                                                    extra, &np,
                                                    string_end - n);
                            if (idx2 != 0
                                && (idx >> 24) == (idx2 >> 24)
                                && len == weights[idx2 & 0xffffff])
                              {
                                int cnt = 0;
                                idx &= 0xffffff;
                                idx2 &= 0xffffff;
                                while (cnt < len
                                       && (weights[idx + 1 + cnt]
                                           == weights[idx2 + 1 + cnt]))
                                  ++cnt;
                                if (cnt == len)
                                  goto matched;
                              }
                          }
                      }
                    else
                      {
                        const int32_t *table = (const int32_t *)
                          _NL_CURRENT (LC_COLLATE, _NL_COLLATE_TABLEWC);
                        const int32_t *weights = (const int32_t *)
                          _NL_CURRENT (LC_COLLATE, _NL_COLLATE_WEIGHTWC);
                        const wint_t *extra = (const wint_t *)
                          _NL_CURRENT (LC_COLLATE, _NL_COLLATE_EXTRAWC);
                        const int32_t *indirect = (const int32_t *)
                          _NL_CURRENT (LC_COLLATE, _NL_COLLATE_INDIRECTWC);
                        wint_t ch = eqch.wc;
                        const wint_t *cp = &ch;
                        int32_t idx = findidxwc (table, indirect, extra,
                                                 &cp, 1);
                        if (idx != 0)
                          {
                            int len = weights[idx & 0xffffff];
                            wint_t nch = nc.wc;
                            const wint_t *np = &nch;
                            int32_t idx2 = findidxwc (table, indirect,
                                                      extra, &np, 1);
                            if (idx2 != 0
                                && (idx >> 24) == (idx2 >> 24)
                                && len == weights[idx2 & 0xffffff])
                              {
                                int cnt = 0;
                                idx &= 0xffffff;
                                idx2 &= 0xffffff;
                                while (cnt < len
                                       && (weights[idx + 1 + cnt]
                                           == weights[idx2 + 1 + cnt]))
                                  ++cnt;
                                if (cnt == len)
                                  goto matched;
                              }
                          }
                      }

                    pc2 = (unsigned char) *p++;
                  }
#endif
                else if (pc2 == '\0')
                  {
                    /* [ unterminated, treat as normal character.  */
                    p = p_init;
                    n = n_init;
                    nps = saved_nps;
                    if (n == string_end)
                      return FNM_NOMATCH;
                    /* Match '[' literally against the current string char.  */
                    struct fnm_char strc = fnm_decode (&n, string_end,
                                                       &nps, mb_cur_max);
                    if (FOLD (strc.wc) != FOLD ((wint_t) '['))
                      return FNM_NOMATCH;
                    ++p;  /* Past the '['.  */
                    goto bracket_done;
                  }
                else
                  {
#ifdef _LIBC
                    is_seqval = false;
                    if (pc2 == '[' && *p == '.')
                      {
                        /* Collating element [.X.].  */
                        uint32_t nrules =
                          _NL_CURRENT_WORD (LC_COLLATE, _NL_COLLATE_NRULES);
                        const char *startp = p;
                        size_t c1 = 0;

                        while (1)
                          {
                            unsigned char cc = (unsigned char) *++p;
                            if (cc == '.' && p[1] == ']')
                              {
                                p += 2;
                                break;
                              }
                            if (cc == '\0')
                              return FNM_NOMATCH;
                            ++c1;
                          }

                        bool is_range = *p == '-' && p[1] != '\0';

                        if (nrules == 0)
                          {
                            if (c1 != 1)
                              return FNM_NOMATCH;
                            if (!is_range
                                && (unsigned char) *n
                                   == (unsigned char) startp[1])
                              goto matched;

                            cold = (unsigned char) startp[1];
                            pc2 = (unsigned char) *p++;
                          }
                        else
                          {
                            int32_t table_size = _NL_CURRENT_WORD
                              (LC_COLLATE, _NL_COLLATE_SYMB_HASH_SIZEMB);
                            const int32_t *symb_table = (const int32_t *)
                              _NL_CURRENT (LC_COLLATE,
                                           _NL_COLLATE_SYMB_TABLEMB);
                            const unsigned char *extra =
                              (const unsigned char *)
                              _NL_CURRENT (LC_COLLATE,
                                           _NL_COLLATE_SYMB_EXTRAMB);
                            int32_t idx = 0;
                            int32_t elem;

                            for (elem = 0; elem < table_size; elem++)
                              if (symb_table[2 * elem] != 0)
                                {
                                  idx = symb_table[2 * elem + 1];
                                  idx += 1 + extra[idx];
                                  if (c1 == extra[idx]
                                      && memcmp (startp + 1,
                                                 &extra[idx + 1], c1) == 0)
                                    break;
                                }

                            if (elem < table_size)
                              {
                                if (!is_range
                                    && string_end - n >= (ptrdiff_t) c1
                                    && memcmp (n, &extra[idx + 1], c1) == 0)
                                  {
                                    /* The post-matched code will advance n
                                       by nc.bytes; consume the rest here.  */
                                    n += c1 - nc.bytes;
                                    /* Reset state since we consumed
                                       multiple bytes.  */
                                    memset (&nps, '\0', sizeof (nps));
                                    goto matched;
                                  }

                                is_seqval = true;
                                idx += 1 + extra[idx];
                                idx = (idx + 3) & ~3;
                                cold = *((int32_t *) &extra[idx]);

                                pc2 = (unsigned char) *p++;
                              }
                            else if (c1 == 1)
                              {
                                if (!is_range
                                    && (unsigned char) *n
                                       == (unsigned char) startp[1])
                                  goto matched;

                                cold = (unsigned char) startp[1];
                                pc2 = (unsigned char) *p++;
                              }
                            else
                              return FNM_NOMATCH;
                          }
                      }
                    else
#endif
                      {
                        /* The character class / equivalence class / collation
                           fallbacks jump here with pc2 == '[', so the ASCII
                           branch below recomputes bracket_c as FOLD ('[').  */
                      normal_bracket:
                        if (pc2 < 0x80)
                          bracket_c = FOLD ((wint_t) pc2);
                        else
                          {
                            /* Multibyte literal char in bracket; decode.  */
                            const char *lit_p = p - 1;
                            mbstate_t ts;
                            memset (&ts, '\0', sizeof (ts));
                            struct fnm_char pat = pat_next (&lit_p, &ts,
                                                            mb_cur_max);
                            bracket_c = FOLD (pat.wc);
                            p = lit_p;
                          }
                        bool is_range = (*p == '-' && p[1] != '\0'
                                         && p[1] != ']');

                        if (!is_range && bracket_c == fn)
                          goto matched;

#ifdef _LIBC
                        is_seqval = false;
#endif
                        cold = bracket_c;
                        pc2 = (unsigned char) *p++;
                      }

                    if (pc2 == '-' && *p != ']')
                      {
                      handle_range:
                        ;
#ifdef _LIBC
                        uint32_t fcollseq;
                        uint32_t lcollseq;
                        wint_t cend;
                        bool cend_is_seqval = false;

                        if (mb_cur_max == 1)
                          {
                            fcollseq = collseqmb[fn & 0xff];
                            lcollseq = is_seqval
                              ? cold : collseqmb[cold & 0xff];
                          }
                        else
                          {
                            /* Use wide collation sequence.  */
                            if (fn < FNM_BAD_BYTE_BASE)
                              fcollseq = __collseq_table_lookup (collseqwc,
                                                                 fn);
                            else
                              fcollseq = ~((uint32_t) 0);
                            if (fcollseq == ~((uint32_t) 0))
                              goto range_not_matched;
                            if (is_seqval)
                              lcollseq = cold;
                            else if (cold < FNM_BAD_BYTE_BASE)
                              lcollseq = __collseq_table_lookup (collseqwc,
                                                                 cold);
                            else
                              lcollseq = ~((uint32_t) 0);
                          }

                        is_seqval = false;
                        /* Read upper bound of range.  */
                        if ((unsigned char) *p == '[' && p[1] == '.')
                          {
                            ++p;  /* Past '['.  */
                            uint32_t nrules = _NL_CURRENT_WORD
                              (LC_COLLATE, _NL_COLLATE_NRULES);
                            const char *startp = p;
                            size_t c1 = 0;

                            while (1)
                              {
                                unsigned char cc = (unsigned char) *++p;
                                if (cc == '.' && p[1] == ']')
                                  {
                                    p += 2;
                                    break;
                                  }
                                if (cc == '\0')
                                  return FNM_NOMATCH;
                                ++c1;
                              }

                            if (nrules == 0)
                              {
                                if (c1 != 1)
                                  return FNM_NOMATCH;
                                cend = (unsigned char) startp[1];
                              }
                            else
                              {
                                int32_t table_size = _NL_CURRENT_WORD
                                  (LC_COLLATE, _NL_COLLATE_SYMB_HASH_SIZEMB);
                                const int32_t *symb_table = (const int32_t *)
                                  _NL_CURRENT (LC_COLLATE,
                                               _NL_COLLATE_SYMB_TABLEMB);
                                const unsigned char *extra =
                                  (const unsigned char *)
                                  _NL_CURRENT (LC_COLLATE,
                                               _NL_COLLATE_SYMB_EXTRAMB);
                                int32_t idx = 0;
                                int32_t elem;

                                for (elem = 0; elem < table_size; elem++)
                                  if (symb_table[2 * elem] != 0)
                                    {
                                      idx = symb_table[2 * elem + 1];
                                      idx += 1 + extra[idx];
                                      if (c1 == extra[idx]
                                          && memcmp (startp + 1,
                                                     &extra[idx + 1], c1)
                                              == 0)
                                        break;
                                    }

                                if (elem < table_size)
                                  {
                                    cend_is_seqval = true;
                                    idx += 1 + extra[idx];
                                    idx = (idx + 3) & ~3;
                                    cend = *((int32_t *) &extra[idx]);
                                  }
                                else if (c1 == 1)
                                  cend = (unsigned char) startp[1];
                                else
                                  return FNM_NOMATCH;
                              }
                          }
                        else
                          {
                            if (!(flags & FNM_NOESCAPE)
                                && (unsigned char) *p == '\\')
                              {
                                ++p;
                                if (*p == '\0')
                                  return FNM_NOMATCH;
                              }
                            if (*p == '\0')
                              return FNM_NOMATCH;
                            unsigned char cb = (unsigned char) *p;
                            if (cb < 0x80)
                              {
                                cend = cb;
                                ++p;
                              }
                            else
                              {
                                const char *lit_p = p;
                                mbstate_t ts;
                                memset (&ts, '\0', sizeof (ts));
                                struct fnm_char pat = pat_next (&lit_p, &ts,
                                                                mb_cur_max);
                                cend = pat.wc;
                                p = lit_p;
                              }
                            cend = FOLD (cend);
                          }

                        /* Now perform the range check using collation
                           sequence values.  */
                        if (lcollseq <= fcollseq)
                          {
                            uint32_t hcollseq;
                            if (cend_is_seqval)
                              hcollseq = cend;
                            else if (mb_cur_max == 1)
                              hcollseq = collseqmb[cend & 0xff];
                            else if (cend < FNM_BAD_BYTE_BASE)
                              hcollseq = __collseq_table_lookup (collseqwc,
                                                                 cend);
                            else
                              hcollseq = ~((uint32_t) 0);
                            if (mb_cur_max != 1
                                && hcollseq == ~((uint32_t) 0))
                              {
                                if (lcollseq != fcollseq)
                                  goto range_not_matched;
                                goto matched;
                              }
                            if (lcollseq <= hcollseq && fcollseq <= hcollseq)
                              goto matched;
                          }
                      range_not_matched:
                        ;
#else
                        /* Outside glibc there is no collation-sequence data,
                           so fall back to a plain comparison of the (folded)
                           character values, like the historical gnulib code.  */
                        wint_t cend;
                        if (!(flags & FNM_NOESCAPE)
                            && (unsigned char) *p == '\\')
                          {
                            ++p;
                            if (*p == '\0')
                              return FNM_NOMATCH;
                          }
                        if (*p == '\0')
                          return FNM_NOMATCH;
                        if ((unsigned char) *p < 0x80)
                          cend = (unsigned char) *p++;
                        else
                          {
                            const char *lit_p = p;
                            mbstate_t ts;
                            memset (&ts, '\0', sizeof (ts));
                            struct fnm_char pat = pat_next (&lit_p, &ts,
                                                            mb_cur_max);
                            cend = pat.wc;
                            p = lit_p;
                          }
                        cend = FOLD (cend);
                        if (cold <= fn && fn <= cend)
                          goto matched;
#endif
                        pc2 = (unsigned char) *p++;
                      }
                  }

                if (pc2 == ']')
                  break;
              }

            if (!not)
              return FNM_NOMATCH;
            /* Negated bracket: advance string and continue.  Here NPS still
               equals SAVED_NPS (the collating fast path jumps to "matched"),
               so consuming NC directly updates the state correctly.  */
            fnm_consume (&n, nc, &nps, mb_cur_max);
            goto bracket_done;

          matched:
            /* Skip the rest of the [...] that already matched.  */
            while ((pc2 = (unsigned char) *p++) != ']')
              {
                if (pc2 == '\0')
                  {
                    p = p_init;
                    n = n_init;
                    nps = saved_nps;
                    if (n == string_end)
                      return FNM_NOMATCH;
                    struct fnm_char strc = fnm_decode (&n, string_end,
                                                       &nps, mb_cur_max);
                    if (FOLD (strc.wc) != FOLD ((wint_t) '['))
                      return FNM_NOMATCH;
                    ++p;
                    goto bracket_done;
                  }
                if (!(flags & FNM_NOESCAPE) && pc2 == '\\')
                  {
                    if (*p == '\0')
                      return FNM_NOMATCH;
                    ++p;
                  }
                else if (pc2 == '[' && *p == ':')
                  {
                    int c1 = 0;
                    const char *startp = p;
                    while (1)
                      {
                        ++p;
                        if (++c1 == CHAR_CLASS_MAX_LENGTH)
                          return FNM_NOMATCH;
                        if (*p == ':' && p[1] == ']')
                          break;
                        if ((unsigned char) *p < 'a'
                            || (unsigned char) *p >= 'z')
                          {
                            p = startp - 2;
                            break;
                          }
                      }
                    p += 2;
                  }
                else if (pc2 == '[' && *p == '=')
                  {
                    ++p;
                    if (*p == '\0')
                      return FNM_NOMATCH;
                    ++p;
                    if (*p != '=' || p[1] != ']')
                      return FNM_NOMATCH;
                    p += 2;
                  }
                else if (pc2 == '[' && *p == '.')
                  {
                    while (1)
                      {
                        ++p;
                        if (*p == '\0')
                          return FNM_NOMATCH;
                        if (*p == '.' && p[1] == ']')
                          break;
                      }
                    p += 2;
                  }
              }
            if (not)
              return FNM_NOMATCH;
            /* Matched: advance string by one character.  */
            n += nc.bytes;
            if (mb_cur_max != 1 && nc.wc >= 0x80 && nc.wc < FNM_BAD_BYTE_BASE)
              {
                wchar_t wc;
                __mbrtowc (&wc, n - nc.bytes, nc.bytes, &saved_nps);
                nps = saved_nps;
              }
            else if (nc.wc >= FNM_BAD_BYTE_BASE)
              memset (&nps, '\0', sizeof (nps));
          bracket_done:
            ;
          }
          break;

        case '+':
        case '@':
        case '!':
          if (__glibc_unlikely (flags & FNM_EXTMATCH) && p[1] == '(')
            {
              int res = ext_match (pc, p + 1, n, string_end,
                                   no_leading_period, flags, mb_cur_max);
              if (res != -1)
                return res;
            }
          ++p;
          goto normal_match_single;

        case '/':
          ++p;
          if (NO_LEADING_PERIOD (flags))
            {
              if (n == string_end || (unsigned char) *n != '/')
                return FNM_NOMATCH;
              ++n;
              memset (&nps, '\0', sizeof (nps));
              new_no_leading_period = true;
              break;
            }
          goto normal_match_single_with_byte;

        default:
          /* Literal character (possibly multibyte).  */
          if (pc < 0x80)
            {
              ++p;
            normal_match_single_with_byte:
              ;
              if (n == string_end)
                return FNM_NOMATCH;
              {
                struct fnm_char nc = fnm_decode (&n, string_end, &nps,
                                                 mb_cur_max);
                if (FOLD (nc.wc) != FOLD ((wint_t) pc))
                  return FNM_NOMATCH;
              }
              break;
            }
          else
            {
              /* Multibyte literal.  PC is only the leading byte, so compare
                 against the fully decoded pattern character.  */
              struct fnm_char pat = pat_next (&p, &pps, mb_cur_max);
              if (n == string_end)
                return FNM_NOMATCH;
              struct fnm_char nc = fnm_decode (&n, string_end, &nps,
                                               mb_cur_max);
              if (FOLD (nc.wc) != FOLD (pat.wc))
                return FNM_NOMATCH;
              break;
            }

        normal_match_single:
          if (n == string_end)
            return FNM_NOMATCH;
          {
            struct fnm_char nc = fnm_decode (&n, string_end, &nps,
                                             mb_cur_max);
            if (FOLD (nc.wc) != FOLD ((wint_t) pc))
              return FNM_NOMATCH;
          }
          break;
        }

      no_leading_period = new_no_leading_period;
    }

  if (n == string_end)
    return 0;

  if ((flags & FNM_LEADING_DIR) && n != string_end && *n == '/')
    return 0;

  return FNM_NOMATCH;
}

/* Find the matching closing ')' for an extended pattern starting at
   PATTERN[1].  Returns a pointer past the ')' on success, or PATTERN
   itself on syntax error.  */
static const char *
end_pattern (const char *pattern)
{
  const char *p = pattern;

  while (1)
    if (*++p == '\0')
      return pattern;
    else if (*p == '[')
      {
        if (posixly_correct == 0)
          posixly_correct = getenv ("POSIXLY_CORRECT") != NULL ? 1 : -1;
        if (*++p == '!' || (posixly_correct < 0 && *p == '^'))
          ++p;
        if (*p == ']')
          ++p;
        while (*p != ']')
          if (*p++ == '\0')
            return pattern;
      }
    else if ((*p == '?' || *p == '*' || *p == '+' || *p == '@' || *p == '!')
             && p[1] == '(')
      {
        p = end_pattern (p + 1);
        if (*p == '\0')
          return pattern;
      }
    else if (*p == ')')
      break;

  return p + 1;
}


#define DYNARRAY_STRUCT            pattern_list
#define DYNARRAY_ELEMENT_FREE(ptr) free (*ptr)
#define DYNARRAY_ELEMENT           char *
#define DYNARRAY_PREFIX            pattern_list_
#define DYNARRAY_INITIAL_SIZE      8
#include <malloc/dynarray-skeleton.c>

static int
ext_match (int opt, const char *pattern, const char *string,
           const char *string_end, bool no_leading_period, int flags,
           int mb_cur_max)
{
  const char *startp;
  ptrdiff_t level;
  struct pattern_list list;
  size_t pattern_len = strlen (pattern);
  size_t pattern_i = 0;
  const char *p;
  const char *rs;
  int retval = 0;

  pattern_list_init (&list);

  /* Parse the pattern.  Store the individual parts in the list.  */
  level = 0;
  for (startp = p = pattern + 1; level >= 0; ++p)
    if (*p == '\0')
      {
        retval = -1;
        goto out;
      }
    else if (*p == '[')
      {
        if (posixly_correct == 0)
          posixly_correct = getenv ("POSIXLY_CORRECT") != NULL ? 1 : -1;
        if (*++p == '!' || (posixly_correct < 0 && *p == '^'))
          ++p;
        if (*p == ']')
          ++p;
        while (*p != ']')
          if (*p++ == '\0')
            {
              retval = -1;
              goto out;
            }
      }
    else if ((*p == '?' || *p == '*' || *p == '+' || *p == '@' || *p == '!')
             && p[1] == '(')
      ++level;
    else if (*p == ')' || *p == '|')
      {
        if (level == 0)
          {
            size_t slen = opt == '?' || opt == '@'
              ? pattern_len : p - startp + 1;
            char *newp = malloc (slen * sizeof (char));
            if (newp != NULL)
              {
                *((char *) __mempcpy (newp, startp, p - startp)) = '\0';
                pattern_list_add (&list, newp);
              }
            if (newp == NULL || pattern_list_has_failed (&list))
              {
                retval = -2;
                goto out;
              }

            if (*p == '|')
              startp = p + 1;
          }
        if (*p == ')')
          level--;
      }
  assert (p[-1] == ')');

  int flags_no_period = flags & FNM_FILE_NAME ? flags : flags & ~FNM_PERIOD;

  switch (opt)
    {
    case '*':
      if (internal_fnmatch (p, string, string_end, no_leading_period,
                            flags, NULL, mb_cur_max) == 0)
        goto success;
      /* Fall through.  */
    case '+':
      for (; pattern_i < pattern_list_size (&list); pattern_i++)
        {
          for (rs = string; rs <= string_end; ++rs)
            if (internal_fnmatch (*pattern_list_at (&list, pattern_i),
                                  string, rs, no_leading_period,
                                  flags_no_period, NULL, mb_cur_max) == 0
                && (internal_fnmatch (p, rs, string_end,
                                      rs == string
                                      ? no_leading_period
                                      : rs[-1] == '/'
                                          && NO_LEADING_PERIOD (flags),
                                      flags_no_period, NULL,
                                      mb_cur_max) == 0
                    || (rs != string
                        && internal_fnmatch (pattern - 1, rs, string_end,
                                             rs == string
                                             ? no_leading_period
                                             : rs[-1] == '/'
                                                 && NO_LEADING_PERIOD (flags),
                                             flags_no_period, NULL,
                                             mb_cur_max) == 0)))
              goto success;
        }
      retval = FNM_NOMATCH;
      break;

    case '?':
      if (internal_fnmatch (p, string, string_end, no_leading_period,
                            flags, NULL, mb_cur_max) == 0)
        goto success;
      /* Fall through.  */
    case '@':
      for (; pattern_i < pattern_list_size (&list); pattern_i++)
        {
          if (internal_fnmatch (strcat (*pattern_list_at (&list, pattern_i),
                                        p),
                                string, string_end, no_leading_period,
                                flags_no_period, NULL, mb_cur_max) == 0)
            goto success;
        }
      retval = FNM_NOMATCH;
      break;

    case '!':
      for (rs = string; rs <= string_end; ++rs)
        {
          size_t runp_i;
          for (runp_i = pattern_i; runp_i != pattern_list_size (&list);
               runp_i++)
            if (internal_fnmatch (*pattern_list_at (&list, runp_i),
                                  string, rs, no_leading_period,
                                  flags_no_period, NULL, mb_cur_max) == 0)
              break;
          if (runp_i == pattern_list_size (&list)
              && (internal_fnmatch (p, rs, string_end,
                                    rs == string
                                    ? no_leading_period
                                    : rs[-1] == '/'
                                        && NO_LEADING_PERIOD (flags),
                                    flags_no_period, NULL,
                                    mb_cur_max) == 0))
            goto success;
        }
      retval = FNM_NOMATCH;
      break;

    default:
      assert (!"Invalid extended matching operator");
      retval = -1;
      break;
    }

 success:
 out:
  pattern_list_free (&list);
  return retval;
}

int
__fnmatch (const char *pattern, const char *string, int flags)
{
#ifdef _LIBC
  int mb_cur_max = _NL_CURRENT_WORD (LC_CTYPE, _NL_CTYPE_MB_CUR_MAX);
#else
  int mb_cur_max = MB_CUR_MAX;
#endif

  return internal_fnmatch (pattern, string, string + strlen (string),
                           flags & FNM_PERIOD, flags, NULL, mb_cur_max);
}

#ifdef _LIBC
versioned_symbol (libc, __fnmatch, fnmatch, GLIBC_2_2_3);
# if SHLIB_COMPAT(libc, GLIBC_2_0, GLIBC_2_2_3)
strong_alias (__fnmatch, __fnmatch_old)
compat_symbol (libc, __fnmatch_old, fnmatch, GLIBC_2_0);
# endif
libc_hidden_ver (__fnmatch, fnmatch)
#endif
