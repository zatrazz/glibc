# Copyright (C) 1991-2025 Free Software Foundation, Inc.
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
#	Makefile for time routines
#
subdir	:= time

include ../Makeconfig

headers := time.h sys/time.h sys/timeb.h bits/time.h			\
	   bits/types/clockid_t.h bits/types/clock_t.h			\
	   bits/types/struct_itimerspec.h				\
	   bits/types/struct_timespec.h bits/types/struct_timeval.h	\
	   bits/types/struct_tm.h bits/types/timer_t.h			\
	   bits/types/time_t.h bits/types/struct_timeb.h

routines := offtime asctime clock ctime ctime_r difftime \
	    gmtime localtime mktime time		 \
	    gettimeofday settimeofday settimezone	 \
	    adjtime tzset tzfile getitimer setitimer	 \
	    stime dysize timegm ftime			 \
	    getdate strptime strptime_l			 \
	    strftime wcsftime strftime_l wcsftime_l	 \
	    timespec_get timespec_getres		 \
	    clock_getcpuclockid clock_getres		 \
	    clock_gettime clock_settime clock_nanosleep

aux :=	    era alt_digit lc-time-cleanup

tests := \
  bug-asctime \
  bug-asctime_r \
  bug-getdate1 \
  bug-mktime1 \
  bug-mktime4 \
  clocktest \
  test_time \
  tst-adjtime \
  tst-clock \
  tst-clock2 \
  tst-clock_gettime \
  tst-clock_nanosleep \
  tst-clock_settime \
  tst-cpuclock1 \
  tst-ctime \
  tst-difftime \
  tst-ftime \
  tst-ftime_l \
  tst-getdate \
  tst-gettimeofday \
  tst-gmtime \
  tst-itimer \
  tst-mktime \
  tst-mktime2 \
  tst-mktime3 \
  tst-mktime4 \
  tst-posixtz \
  tst-settimeofday \
  tst-strftime \
  tst-strftime2 \
  tst-strftime3 \
  tst-strftime4 \
  tst-strptime \
  tst-strptime-whitespace \
  tst-strptime2 \
  tst-strptime3 \
  tst-time \
  tst-timegm \
  tst-timespec_get \
  tst-timespec_getres \
  tst-tzfile-fault \
  tst-tzname \
  tst-y2039 \
  tst_wcsftime \
  # tests

tests-time64 := \
  tst-adjtime-time64 \
  tst-clock-time64 \
  tst-clock2-time64 \
  tst-clock_gettime-time64 \
  tst-clock_nanosleep-time64 \
  tst-clock_settime-time64 \
  tst-cpuclock1-time64 \
  tst-ctime-time64 \
  tst-difftime-time64 \
  tst-gettimeofday-time64 \
  tst-gmtime-time64 \
  tst-itimer-time64 \
  tst-mktime4-time64 \
  tst-settimeofday-time64 \
  tst-strftime4-time64 \
  tst-time-time64 \
  tst-timegm-time64 \
  tst-timespec_get-time64 \
  tst-timespec_getres-time64 \
  tst-y2039-time64

include ../Rules

ifeq ($(run-built-tests),yes)
LOCALES := de_DE.ISO-8859-1 en_US.ISO-8859-1 ja_JP.EUC-JP fr_FR.UTF-8 \
	   es_ES.UTF-8 pl_PL.UTF-8 ru_RU.UTF-8 \
	   ja_JP.UTF-8 lo_LA.UTF-8 th_TH.UTF-8 \
	   zh_TW.UTF-8 cmn_TW.UTF-8 hak_TW.UTF-8 \
	   nan_TW.UTF-8 lzh_TW.UTF-8
include ../gen-locales.mk

$(objpfx)tst-ftime_l.out: $(gen-locales)
$(objpfx)tst-strptime.out: $(gen-locales)
$(objpfx)tst-strftime2.out: $(gen-locales)
$(objpfx)tst-strftime3.out: $(gen-locales)
endif

$(objpfx)tst-clock_gettime: $(librt)
$(objpfx)tst-clock_gettime-time64: $(librt)
$(objpfx)tst-clock_nanosleep: $(librt)
$(objpfx)tst-clock_nanosleep-time64: $(librt)

tz-cflags = -DTZDIR='"$(zonedir)"' \
	    -DTZDEFAULT='"$(localtime-file)"' \
	    -DTZDEFRULES='"$(posixrules-file)"'

CFLAGS-tzfile.c += $(tz-cflags)
CFLAGS-tzset.c += $(tz-cflags)
CFLAGS-getdate.c += -fexceptions
CFLAGS-clock_nanosleep.c += -fexceptions -fasynchronous-unwind-tables
CFLAGS-nanosleep.c += -fexceptions -fasynchronous-unwind-tables
CFLAGS-mktime.c += $(config-cflags-wno-ignored-attributes)

# Don't warn about Y2k problem in strftime format string.
CFLAGS-test_time.c += -Wno-format

test_time-ARGS= EST5EDT CST

tst-tzname-ENV = TZDIR=${common-objpfx}timezone/testdata
CPPFLAGS-tst-tzname.c += -DTZDEFRULES='"$(posixrules-file)"'

bug-getdate1-ARGS = ${objpfx}bug-getdate1-fmt

tst-tzfile-fault-ENV = GLIBC_TUNABLES=glibc.rtld.enable_secure=1
