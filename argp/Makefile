# Copyright (C) 1997-2026 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with the GNU C Library; if not, see
# <https://www.gnu.org/licenses/>.

#
#	Makefile for argp.
#
subdir	:= argp

include ../Makeconfig

headers = \
  argp.h \
  bits/argp-ldbl.h \
  # headers
routines = \
  argp-ba \
  argp-eexst \
  argp-fmtstream \
  argp-fs-xinl \
  argp-help \
  argp-parse \
  argp-pv \
  argp-pvh \
  argp-xinl \
  # routines

tests = \
  argp-test \
  bug-argp1 \
  bug-argp2 \
  tst-argp1 \
  tst-argp2 \
  tst-argphelp-localized \
  tst-argpusage-localized \
  tst-ldbl-argp \
  # tests

CFLAGS-argp-help.c += $(uses-callbacks) -fexceptions
CFLAGS-argp-parse.c += $(uses-callbacks)
CFLAGS-argp-fmtstream.c += -fexceptions

tst_argphelp_localized_mo = $(objpfx)domaindir/en_GB/LC_MESSAGES/tst-argphelp-localized.mo

$(tst_argphelp_localized_mo): tst-argphelp-localized.po
	$(make-target-directory)
	msgfmt -o $@T $<
	mv -f $@T $@

LOCALES := \
  en_GB.UTF-8 \
  # LOCALES
include ../gen-locales.mk

$(objpfx)tst-argphelp-localized.out: $(tst_argphelp_localized_mo) $(gen-locales)
$(objpfx)tst-argpusage-localized.out: $(tst_argphelp_localized_mo) $(gen-locales)
CFLAGS-tst-argphelp-localized.c += -DOBJPFX=\"$(objpfx)\"
CFLAGS-tst-argpusage-localized.c += -DOBJPFX=\"$(objpfx)\"

bug-argp1-ARGS = -- --help
bug-argp2-ARGS = -- -d 111 --dstaddr 222 -p 333 --peer 444

include ../Rules
