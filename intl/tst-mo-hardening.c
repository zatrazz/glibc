/* Test that malformed/malicious .mo catalogs are rejected safely.
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

/* The .mo file header contains a number of offsets, counts and lengths
   that are used to index into the memory-mapped file.  A corrupt or
   malicious catalog can set these so that the parser in
   intl/loadmsgcat.c reads or writes out of bounds:

     - on ILP32 the sum of the sysdep segment sizes overflows size_t,
       producing an undersized allocation followed by a large memcpy
       (heap overflow);
     - a sysdep string offset (or any table offset) pointing past the
       end of the file causes an out-of-bounds read.

   This test crafts such catalogs and checks that gettext() rejects them
   gracefully -- returning the untranslated msgid and, above all, not
   crashing -- while still loading well-formed catalogs, including ones
   using system-dependent (ISO C99 <inttypes.h>) string segments.  */

#include <inttypes.h>
#include <locale.h>
#include <libintl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <array_length.h>
#include <support/check.h>
#include <support/support.h>
#include <support/temp_file.h>
#include <support/xstdio.h>
#include <support/xunistd.h>

/* GNU message catalog constants (see intl/gmo.h).  */
#define MO_MAGIC	0x950412de
#define SEGMENTS_END	0xffffffffU

/* Byte offsets of the fields in struct mo_file_header.  */
#define H_MAGIC		0
#define H_REVISION	4
#define H_NSTRINGS	8
#define H_ORIG_TAB	12
#define H_TRANS_TAB	16
#define H_HASH_SIZE	20
#define H_HASH_TAB	24
#define H_N_SYSDEP_SEG	28
#define H_SYSDEP_SEG	32
#define H_N_SYSDEP_STR	36
#define H_ORIG_SYSDEP	40
#define H_TRANS_SYSDEP	44
#define MO_HEADER_SIZE	48

/* The directory catalogs are looked up in.  */
static const char *domaindir;

static void
put32 (unsigned char *buf, size_t off, uint32_t val)
{
  buf[off + 0] = val & 0xff;
  buf[off + 1] = (val >> 8) & 0xff;
  buf[off + 2] = (val >> 16) & 0xff;
  buf[off + 3] = (val >> 24) & 0xff;
}

/* Write BUF (LEN bytes) as domaindir/NAME/LC_MESSAGES/NAME.mo.  Each case
   uses NAME as both the LANGUAGE (locale subdirectory) and the message
   domain name so that __dcigettext's per-(msgid,domain) translation cache
   never conflates two cases.  */
static void
write_catalog (const char *name, const unsigned char *buf, size_t len)
{
  char *ldir = xasprintf ("%s/%s", domaindir, name);
  char *dir = xasprintf ("%s/%s/LC_MESSAGES", domaindir, name);
  xmkdirp (dir, 0777);
  char *path = xasprintf ("%s/%s.mo", dir, name);
  FILE *f = xfopen (path, "wb");
  TEST_COMPARE (fwrite (buf, 1, len, f), len);
  xfclose (f);

  /* Register the created paths so the temporary tree is removed cleanly.
     Files are removed most-recently-added first, so register the parent
     directories before the file they contain.  */
  add_temp_file (ldir);
  add_temp_file (dir);
  add_temp_file (path);

  free (path);
  free (dir);
  free (ldir);
}

/* Look up MSGID in the catalog named NAME.  */
static const char *
lookup (const char *name, const char *msgid)
{
  setenv ("LANGUAGE", name, 1);
  bindtextdomain (name, domaindir);
  bind_textdomain_codeset (name, "UTF-8");
  return dgettext (name, msgid);
}

static size_t
align4 (size_t x)
{
  return (x + 3) & ~(size_t) 3;
}

/* A valid revision-0 catalog translating "hello" -> "bonjour", with a
   proper "" header entry so charset handling works.  */
static size_t
build_valid_plain (unsigned char *b)
{
  static const char header[] = "Content-Type: text/plain; charset=UTF-8\n";
  size_t hlen = strlen (header);

  memset (b, 0, 1024);
  put32 (b, H_MAGIC, MO_MAGIC);
  put32 (b, H_REVISION, 0);
  put32 (b, H_NSTRINGS, 2);

  size_t otab = MO_HEADER_SIZE;		/* two struct string_desc.  */
  size_t ttab = otab + 2 * 8;
  put32 (b, H_ORIG_TAB, otab);
  put32 (b, H_TRANS_TAB, ttab);
  put32 (b, H_HASH_SIZE, 0);		/* No hash table -> binary search.  */
  put32 (b, H_HASH_TAB, 0);

  size_t cur = ttab + 2 * 8;

  /* Strings, in strcmp order for the binary search: "" then "hello".  */
  size_t empty_off = cur;
  b[cur++] = '\0';
  size_t header_off = cur;
  memcpy (b + cur, header, hlen + 1);
  cur += hlen + 1;
  size_t hello_off = cur;
  memcpy (b + cur, "hello", 6);
  cur += 6;
  size_t bonjour_off = cur;
  memcpy (b + cur, "bonjour", 8);
  cur += 8;

  /* orig_tab[0] = "", orig_tab[1] = "hello".  */
  put32 (b, otab + 0, 0);
  put32 (b, otab + 4, empty_off);
  put32 (b, otab + 8, 5);
  put32 (b, otab + 12, hello_off);
  /* trans_tab[0] = header, trans_tab[1] = "bonjour".  */
  put32 (b, ttab + 0, hlen);
  put32 (b, ttab + 4, header_off);
  put32 (b, ttab + 8, 7);
  put32 (b, ttab + 12, bonjour_off);

  return cur;
}

/* A valid revision-1 catalog with one system-dependent string, so the
   sysdep code path (the one carrying the reported bugs) is exercised on
   well-formed input too.  The msgid expands to "a" PRId8 "b" and its
   translation to "X" PRId8 "Y" on the running platform.  */
static size_t
build_valid_sysdep (unsigned char *b)
{
  static const char header[] = "Content-Type: text/plain; charset=UTF-8\n";
  size_t hlen = strlen (header);

  memset (b, 0, 1024);
  put32 (b, H_MAGIC, MO_MAGIC);
  put32 (b, H_REVISION, 1);		/* major 0, minor 1.  */
  put32 (b, H_NSTRINGS, 1);		/* Just the "" header entry.  */
  put32 (b, H_N_SYSDEP_SEG, 1);
  put32 (b, H_N_SYSDEP_STR, 1);

  size_t otab = MO_HEADER_SIZE;		/* one struct string_desc.  */
  size_t ttab = otab + 8;
  size_t hash = ttab + 8;		/* five nls_uint32.  */
  put32 (b, H_ORIG_TAB, otab);
  put32 (b, H_TRANS_TAB, ttab);
  put32 (b, H_HASH_SIZE, 5);
  put32 (b, H_HASH_TAB, hash);
  /* hashpjw ("") == 0, so the "" entry (string index 0, stored as 1) sits
     in slot 0; the sysdep string is added to the augmented table later.  */
  put32 (b, hash + 0, 1);

  size_t cur = hash + 5 * 4;

  /* System dependent segment table + name "PRId8".  */
  size_t segtab = cur;
  put32 (b, H_SYSDEP_SEG, segtab);
  cur += 8;
  size_t segname = cur;
  memcpy (b + cur, "PRId8", 6);
  cur += 6;
  put32 (b, segtab + 0, 6);		/* length incl. NUL.  */
  put32 (b, segtab + 4, segname);

  cur = align4 (cur);
  size_t orig_sysdep_tab = cur;
  put32 (b, H_ORIG_SYSDEP, orig_sysdep_tab);
  cur += 4;
  size_t trans_sysdep_tab = cur;
  put32 (b, H_TRANS_SYSDEP, trans_sysdep_tab);
  cur += 4;

  cur = align4 (cur);
  /* orig sysdep string: static "a", sysdep PRId8, static "b\0".  */
  size_t orig_ss = cur;
  put32 (b, orig_sysdep_tab, orig_ss);
  cur += 4 + 8 + 8;			/* offset + two segment_pair.  */
  size_t trans_ss = cur;
  put32 (b, trans_sysdep_tab, trans_ss);
  cur += 4 + 8 + 8;

  size_t orig_static = cur;
  b[cur++] = 'a';
  b[cur++] = 'b';
  b[cur++] = '\0';
  size_t trans_static = cur;
  b[cur++] = 'X';
  b[cur++] = 'Y';
  b[cur++] = '\0';

  /* Fill the orig sysdep string descriptor.  */
  put32 (b, orig_ss + 0, orig_static);
  put32 (b, orig_ss + 4, 1);		/* seg0 segsize.  */
  put32 (b, orig_ss + 8, 0);		/* seg0 sysdepref -> "PRId8".  */
  put32 (b, orig_ss + 12, 2);		/* seg1 segsize ("b\0").  */
  put32 (b, orig_ss + 16, SEGMENTS_END);
  /* And the trans one.  */
  put32 (b, trans_ss + 0, trans_static);
  put32 (b, trans_ss + 4, 1);
  put32 (b, trans_ss + 8, 0);
  put32 (b, trans_ss + 12, 2);
  put32 (b, trans_ss + 16, SEGMENTS_END);

  /* The "" header lives after the sysdep data.  */
  size_t empty_off = cur;
  b[cur++] = '\0';
  size_t header_off = cur;
  memcpy (b + cur, header, hlen + 1);
  cur += hlen + 1;
  put32 (b, otab + 0, 0);
  put32 (b, otab + 4, empty_off);
  put32 (b, ttab + 0, hlen);
  put32 (b, ttab + 4, header_off);

  return cur;
}

/* Reported 64-bit PoC: a well-formed-looking revision-1 catalog whose
   sysdep string points its static segment 1 MiB past a ~150 byte file.  */
static size_t
build_evil_oob_offset (unsigned char *b)
{
  memset (b, 0, 1024);
  put32 (b, H_MAGIC, MO_MAGIC);
  put32 (b, H_REVISION, 1);
  put32 (b, H_NSTRINGS, 1);
  put32 (b, H_HASH_SIZE, 3);
  put32 (b, H_N_SYSDEP_SEG, 1);
  put32 (b, H_N_SYSDEP_STR, 1);

  size_t otab = MO_HEADER_SIZE, ttab = otab + 8, hash = ttab + 8;
  put32 (b, H_ORIG_TAB, otab);
  put32 (b, H_TRANS_TAB, ttab);
  put32 (b, H_HASH_TAB, hash);
  size_t cur = hash + 3 * 4;

  size_t ostr = cur; b[cur++] = '\0';
  size_t tstr = cur; b[cur++] = '\0';
  put32 (b, otab + 4, ostr);
  put32 (b, ttab + 4, tstr);
  cur = align4 (cur);

  size_t segtab = cur;
  put32 (b, H_SYSDEP_SEG, segtab);
  size_t segname = cur + 8;
  put32 (b, segtab + 0, 6);
  put32 (b, segtab + 4, segname);
  cur += 8;
  memcpy (b + cur, "PRId8", 6);
  cur += 6;
  cur = align4 (cur);

  size_t ostab = cur; put32 (b, H_ORIG_SYSDEP, ostab); cur += 4;
  size_t tstab = cur; put32 (b, H_TRANS_SYSDEP, tstab); cur += 4;
  cur = align4 (cur);

  /* orig sysdep string with an out-of-file static offset.  */
  size_t oss = cur;
  put32 (b, ostab, oss);
  put32 (b, oss + 0, 0x00100000);	/* 1 MiB past EOF.  */
  put32 (b, oss + 4, 256);		/* segsize.  */
  put32 (b, oss + 8, 0);
  put32 (b, oss + 12, 0);
  put32 (b, oss + 16, SEGMENTS_END);
  cur += 20;

  size_t tss = cur;
  put32 (b, tstab, tss);
  size_t tstatic = cur + 20;
  put32 (b, tss + 0, tstatic);
  put32 (b, tss + 4, 4);
  put32 (b, tss + 8, 0);
  put32 (b, tss + 12, 1);
  put32 (b, tss + 16, SEGMENTS_END);
  cur += 20;
  memcpy (b + cur, "OK\0\0", 4);
  cur += 4;
  b[cur++] = '\0';

  return cur;
}

/* Reported 32-bit PoC: two sysdep segment sizes of 0x80000008 whose sum
   wraps size_t on ILP32.  On any width the huge sizes also fall outside
   the file, so the catalog is rejected either way.  */
static size_t
build_evil_overflow (unsigned char *b)
{
  memset (b, 0, 1024);
  put32 (b, H_MAGIC, MO_MAGIC);
  put32 (b, H_REVISION, 1);
  put32 (b, H_NSTRINGS, 1);
  put32 (b, H_HASH_SIZE, 3);
  put32 (b, H_N_SYSDEP_SEG, 1);
  put32 (b, H_N_SYSDEP_STR, 1);

  size_t otab = MO_HEADER_SIZE, ttab = otab + 8, hash = ttab + 8;
  put32 (b, H_ORIG_TAB, otab);
  put32 (b, H_TRANS_TAB, ttab);
  put32 (b, H_HASH_TAB, hash);
  size_t cur = hash + 3 * 4;

  size_t ostr = cur; b[cur++] = '\0';
  size_t tstr = cur; b[cur++] = '\0';
  put32 (b, otab + 4, ostr);
  put32 (b, ttab + 4, tstr);
  cur = align4 (cur);

  size_t segtab = cur;
  put32 (b, H_SYSDEP_SEG, segtab);
  size_t segname = cur + 8;
  put32 (b, segtab + 0, 6);
  put32 (b, segtab + 4, segname);
  cur += 8;
  memcpy (b + cur, "PRId8", 6);
  cur += 6;
  cur = align4 (cur);

  size_t ostab = cur; put32 (b, H_ORIG_SYSDEP, ostab); cur += 4;
  size_t tstab = cur; put32 (b, H_TRANS_SYSDEP, tstab); cur += 4;
  cur = align4 (cur);

  size_t oss = cur;
  put32 (b, ostab, oss);
  put32 (b, oss + 0, oss + 20);		/* static offset (in file).  */
  put32 (b, oss + 4, 0x80000008);	/* seg0 segsize.  */
  put32 (b, oss + 8, 0);		/* -> "PRId8".  */
  put32 (b, oss + 12, 0x80000008);	/* seg1 segsize; sum wraps on ILP32.  */
  put32 (b, oss + 16, SEGMENTS_END);
  cur += 20;
  memset (b + cur, 'A', 64);
  cur += 64;

  size_t tss = cur;
  put32 (b, tstab, tss);
  size_t tstatic = cur + 20;
  put32 (b, tss + 0, tstatic);
  put32 (b, tss + 4, 4);
  put32 (b, tss + 8, 0);
  put32 (b, tss + 12, 1);
  put32 (b, tss + 16, SEGMENTS_END);
  cur += 20;
  memcpy (b + cur, "Btest\0\0\0", 8);
  cur += 8;
  b[cur++] = '\0';

  return cur;
}

/* Revision-0 catalog whose only string descriptor points past EOF; this
   used to reach an out-of-bounds read in _nl_find_msg's strcmp.  */
static size_t
build_base_tab_oob (unsigned char *b)
{
  memset (b, 0, 1024);
  put32 (b, H_MAGIC, MO_MAGIC);
  put32 (b, H_REVISION, 0);
  put32 (b, H_NSTRINGS, 1);
  size_t otab = MO_HEADER_SIZE, ttab = otab + 8;
  put32 (b, H_ORIG_TAB, otab);
  put32 (b, H_TRANS_TAB, ttab);
  /* orig_tab[0].offset way past the end of the file.  */
  put32 (b, otab + 0, 5);
  put32 (b, otab + 4, 0x00100000);
  put32 (b, ttab + 0, 5);
  put32 (b, ttab + 4, 0x00100000);
  return ttab + 8;
}

/* Revision-1 catalog whose sysdep segment table offset is past EOF.  */
static size_t
build_segtab_oob (unsigned char *b)
{
  memset (b, 0, 1024);
  put32 (b, H_MAGIC, MO_MAGIC);
  put32 (b, H_REVISION, 1);
  put32 (b, H_NSTRINGS, 1);
  put32 (b, H_HASH_SIZE, 3);
  put32 (b, H_N_SYSDEP_SEG, 1);
  put32 (b, H_N_SYSDEP_STR, 1);
  size_t otab = MO_HEADER_SIZE, ttab = otab + 8, hash = ttab + 8;
  put32 (b, H_ORIG_TAB, otab);
  put32 (b, H_TRANS_TAB, ttab);
  put32 (b, H_HASH_TAB, hash);
  put32 (b, H_SYSDEP_SEG, 0x00100000);	/* table past EOF.  */
  put32 (b, H_ORIG_SYSDEP, 0x00100000);
  put32 (b, H_TRANS_SYSDEP, 0x00100000);
  size_t cur = hash + 3 * 4;
  size_t ostr = cur; b[cur++] = '\0';
  size_t tstr = cur; b[cur++] = '\0';
  put32 (b, otab + 4, ostr);
  put32 (b, ttab + 4, tstr);
  return cur;
}

/* Revision-0 catalog whose hash slot holds an index far beyond the string
   tables.  _nl_find_msg would decode it and dereference the (NULL) sysdep
   table out of bounds.  */
static size_t
build_hash_index_oob (unsigned char *b)
{
  memset (b, 0, 1024);
  put32 (b, H_MAGIC, MO_MAGIC);
  put32 (b, H_REVISION, 0);
  put32 (b, H_NSTRINGS, 1);
  put32 (b, H_HASH_SIZE, 3);
  size_t otab = MO_HEADER_SIZE, ttab = otab + 8, hash = ttab + 8;
  put32 (b, H_ORIG_TAB, otab);
  put32 (b, H_TRANS_TAB, ttab);
  put32 (b, H_HASH_TAB, hash);
  put32 (b, hash + 0, 99);		/* index 98, but only 1 string.  */
  size_t cur = hash + 3 * 4;
  size_t ostr = cur; memcpy (b + cur, "x", 2); cur += 2;
  size_t tstr = cur; memcpy (b + cur, "y", 2); cur += 2;
  put32 (b, otab + 0, 1); put32 (b, otab + 4, ostr);
  put32 (b, ttab + 0, 1); put32 (b, ttab + 4, tstr);
  return cur;
}

/* Revision-0 catalog whose hash table has no empty slot.  A lookup that
   misses used to probe forever; it must now terminate.  */
static size_t
build_hash_full (unsigned char *b)
{
  memset (b, 0, 1024);
  put32 (b, H_MAGIC, MO_MAGIC);
  put32 (b, H_REVISION, 0);
  put32 (b, H_NSTRINGS, 3);
  put32 (b, H_HASH_SIZE, 3);
  size_t otab = MO_HEADER_SIZE, ttab = otab + 3 * 8, hash = ttab + 3 * 8;
  put32 (b, H_ORIG_TAB, otab);
  put32 (b, H_TRANS_TAB, ttab);
  put32 (b, H_HASH_TAB, hash);
  /* Every slot is occupied and in range (indices 0, 1, 2).  */
  put32 (b, hash + 0, 1);
  put32 (b, hash + 4, 2);
  put32 (b, hash + 8, 3);
  size_t cur = hash + 3 * 4;
  static const char *const os[] = { "aa", "bb", "cc" };
  static const char *const ts[] = { "AA", "BB", "CC" };
  for (int i = 0; i < 3; i++)
    {
      size_t o = cur; memcpy (b + cur, os[i], 3); cur += 3;
      size_t t = cur; memcpy (b + cur, ts[i], 3); cur += 3;
      put32 (b, otab + i * 8 + 0, 2); put32 (b, otab + i * 8 + 4, o);
      put32 (b, ttab + i * 8 + 0, 2); put32 (b, ttab + i * 8 + 4, t);
    }
  return cur;
}

static int
do_test (void)
{
  unsigned char buf[8192];
  size_t len;

  /* A real locale is needed so that LANGUAGE is honoured; LOCPATH is set
     by the Makefile to the freshly built locales.  */
  if (setlocale (LC_ALL, "de_DE.UTF-8") == NULL)
    FAIL_UNSUPPORTED ("cannot set de_DE.UTF-8 locale");
  unsetenv ("LC_ALL");
  unsetenv ("OUTPUT_CHARSET");

  domaindir = support_create_temp_directory ("tst-mo-hardening-");

  /* Positive cases: well-formed catalogs must still work.  */
  len = build_valid_plain (buf);
  write_catalog ("valid_plain", buf, len);
  TEST_COMPARE_STRING (lookup ("valid_plain", "hello"), "bonjour");

  len = build_valid_sysdep (buf);
  write_catalog ("valid_sysdep", buf, len);
  {
    const char *msgid = "a" PRId8 "b";
    const char *expected = "X" PRId8 "Y";
    TEST_COMPARE_STRING (lookup ("valid_sysdep", msgid), expected);
  }

  /* Negative cases: malformed catalogs must be rejected (msgid returned
     unchanged) and, crucially, must not crash.  */
  struct
  {
    const char *lang;
    size_t (*build) (unsigned char *);
  } evil[] =
    {
      { "evil_oob_offset", build_evil_oob_offset },
      { "evil_overflow", build_evil_overflow },
      { "evil_base_tab", build_base_tab_oob },
      { "evil_segtab", build_segtab_oob },
      { "evil_hash_index", build_hash_index_oob },
      { "evil_hash_full", build_hash_full },
    };

  for (size_t i = 0; i < array_length (evil); i++)
    {
      len = evil[i].build (buf);
      write_catalog (evil[i].lang, buf, len);
      TEST_COMPARE_STRING (lookup (evil[i].lang, "hello"), "hello");
    }

  return 0;
}

#include <support/test-driver.c>
