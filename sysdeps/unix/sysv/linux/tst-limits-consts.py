#!/usr/bin/python3
# Test that glibc's limits.h constants match the kernel's.
# Copyright (C) 2022-2023 Free Software Foundation, Inc.
# This file is part of the GNU C Library.
#
# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with the GNU C Library; if not, see
# <https://www.gnu.org/licenses/>.

import argparse
import sys

import glibcextract
import glibcsyscalls


def main():
    """The main entry point."""
    parser = argparse.ArgumentParser(
        description="Test that glibc's limits.h constants "
        "match the kernel's.")
    parser.add_argument('--cc', metavar='CC',
                        help='C compiler (including options) to use')
    args = parser.parse_args()

    def check_single(kcte, cte):
        macros_1 = glibcextract.compute_macro_consts(
                '#include <linux/limits.h>\n',
                args.cc,
                kcte)
        macros_2 = glibcextract.compute_macro_consts(
                '#include <bits/stdlib_lim.h>\n',
                args.cc,
                cte)
        ret = 1
        for (k1, v1), (k2, v2) in zip(macros_1.items(), macros_2.items()):
            if v1 == v2:
                ret = 0
        return ret

    sys.exit(check_single('PATH_MAX', '__PATH_MAX'))

if __name__ == '__main__':
    main()
