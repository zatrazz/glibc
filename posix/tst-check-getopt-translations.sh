#!/bin/sh
# Test for check-getopt-translations.
# Copyright (C) 2026 Free Software Foundation, Inc.
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

set -e

check_getopt_translations_program=$1; shift
po_file=$1; shift
logfile=$1; shift

rm -f $logfile
result=0
expected_output="\
Translation toto is used for more than one option:
  - bar
  - foo
bar is a translation of pub, but it is also a different option.
There were 2 failures."

if output=$(${check_getopt_translations_program} "command-line option" ${po_file} 2>&1) ; then
    echo "the errors were not caught." >> $logfile
    echo "*** check-getopt-translations FAILED" >> $logfile
    result=1
fi

if test "$output" != "$expected_output"; then
    echo "Expected:" >> $logfile
    echo "$expected_output" >> $logfile
    echo "Actual:" >> $logfile
    echo "$output" >> $logfile
    echo "*** check-getopt-translations FAILED" >> $logfile
    result=1
fi

echo "*** check-getopt-translations PASSED" >> $logfile

exit $result

# Preserve executable bits for this shell script.
Local Variables:
eval:(defun frobme () (set-file-modes buffer-file-name file-mode))
eval:(make-local-variable 'file-mode)
eval:(setq file-mode (file-modes (buffer-file-name)))
eval:(make-local-variable 'after-save-hook)
eval:(add-hook 'after-save-hook 'frobme)
End:
