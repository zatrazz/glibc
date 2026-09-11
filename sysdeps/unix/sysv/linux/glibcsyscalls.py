#!/usr/bin/python3
# Helpers for glibc system call list processing.
# Copyright (C) 2018-2026 Free Software Foundation, Inc.
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
# <http://www.gnu.org/licenses/>.

import os
import re

if __name__ != '__main__':
    # When called as a main program, this is not needed.
    import glibcextract

def extract_system_call_name(macro):
    """Convert the macro name (with __NR_) to a system call name."""
    prefix = '__NR_'
    if macro.startswith(prefix):
        return macro[len(prefix):]
    else:
        raise ValueError('invalid system call name: {!r}'.format(macro))

# Matches macros for system call names.
RE_SYSCALL = re.compile('__NR_.*')

# Some __NR_ constants are not real
RE_PSEUDO_SYSCALL = re.compile(r"""__NR_(
    # Reserved system call.
    (unused|reserved)[0-9]+

    # Pseudo-system call which describes a range.
    |(syscalls|arch_specific_syscall|(OABI_)?SYSCALL_BASE|SYSCALL_MASK)
    |(|64_|[NO]32_)Linux(_syscalls)?
   )""", re.X)

# Matches a system call number reference, as in __NR_openat.
RE_SYSCALL_NUMBER = re.compile(r'__NR_(\w+)')

# Matches the name argument of the macros which issue a system call, as
# in INLINE_SYSCALL_CALL (openat, ...).
RE_SYSCALL_MACRO = re.compile(r"""\b(?:
     (?:INLINE|INTERNAL)_V?SYSCALL(?:_CALL|_CANCEL)?
     |INLINE_(?:SETXID|CLONE)_SYSCALL
     |SYSCALL_CANCEL(?:_CALL)?
   )[\s\\]*\([\s\\]*(\w+)""", re.X)

# Matches the name argument of the assembler macros, as in
# PSEUDO (__openat, openat, 4).
RE_SYSCALL_PSEUDO = re.compile(
    r'\bPSEUDO(?:_NOERRNO|_ERRVAL)?[\s\\]*\([\s\\]*\w+[\s\\]*,[\s\\]*(\w+)')

# Headers which list every system call number by construction.
SYSCALL_NUMBER_HEADERS = frozenset(('arch-syscall.h', 'fixup-asm-unistd.h'))

# Legacy system calls which the generic kernel system call ABI lists.
LEGACY_SYSCALLS = frozenset((
    # Wired to sys_ni_syscall in the generic ABI.
    'lookup_dcookie',
    # Pre-generic ABI leftover, provided by ARC only.
    'sysfs',
    # Superseded by tgkill.
    'tkill',
))

def kernel_constants(cc):
    """Return a dictionary with the kernel-defined system call numbers.

    This comes from <asm/unistd.h>.

    """
    return {extract_system_call_name(name) : int(value)
            for name, value in glibcextract.compute_macro_consts(
                    '#include <asm/unistd.h>\n'
                    # Regularize the kernel definitions if necessary.
                    '#include <fixup-asm-unistd.h>',
                    cc, macro_re=RE_SYSCALL, exclude_re=RE_PSEUDO_SYSCALL)
            .items()}

class SyscallNamesList:
    """The list of known system call names.

    glibc keeps a list of system call names.  The <sys/syscall.h>
    header needs to provide a SYS_ name for each __NR_ macro,
    and the generated <bits/syscall.h> header uses an
    architecture-independent list, so that there is a chance that
    system calls arriving late on certain architectures will automatically
    get the expected SYS_ macro.

    syscalls: list of strings with system call names
    kernel_version: tuple of integers; the kernel version given in the file

    """
    def __init__(self, lines):
        self.syscalls = []
        old_name = None
        self.kernel_version = None
        self.__lines = tuple(lines)
        for line in self.__lines:
            line = line.strip()
            if (not line) or line[0] == '#':
                continue
            comps = line.split()
            if len(comps) == 1:
                self.syscalls.append(comps[0])
                if old_name is not None:
                    if comps[0] < old_name:
                        raise ValueError(
                            'name list is not sorted: {!r} < {!r}'.format(
                                comps[0], old_name))
                old_name = comps[0]
                continue
            if len(comps) == 2 and comps[0] == "kernel":
                if self.kernel_version is not None:
                    raise ValueError(
                        "multiple kernel versions: {!r} and !{r}".format(
                            kernel_version, comps[1]))
                self.kernel_version = tuple(map(int, comps[1].split(".")))
                continue
            raise ValueError("invalid line: !r".format(line))
        if self.kernel_version is None:
            raise ValueError("missing kernel version")

    def merge(self, names):
        """Merge sequence NAMES and return the lines of the new file."""
        names = list(set(names) - set(self.syscalls))
        names.sort()
        names.reverse()
        result = []
        def emit_name():
            result.append(names[-1] + "\n")
            del names[-1]

        for line in self.__lines:
            comps = line.strip().split()
            if len(comps) == 1 and not comps[0].startswith("#"):
                # File has a system call at this position.  Insert all
                # the names that come before the name in the file
                # lexicographically.
                while names and names[-1] < comps[0]:
                    emit_name()
            result.append(line)
        while names:
            emit_name()

        return result

def load_arch_syscall_header(path):
    """"Load the system call header form the file PATH.

    The file must consist of lines of this form:

    #define __NR_exit 1

    The file is parsed verbatim, without running it through a C
    preprocessor or parser.  The intent is that the file can be
    readily processed by tools.

    """
    with open(path) as inp:
        result = {}
        old_name = None
        for line in inp:
            line = line.strip()

            # Ignore the initial comment line.
            if line.startswith("/*") and line.endswith("*/"):
                continue

            define, name, number = line.split(' ', 2)
            if define != '#define':
                raise ValueError("invalid syscall header line: {!r}".format(
                    line))
            result[extract_system_call_name(name)] = int(number)

            # Check list order.
            if old_name is not None:
                if name < old_name:
                    raise ValueError(
                        'system call list is not sorted: {!r} < {!r}'.format(
                            name, old_name))
            old_name = name
        return result

def linux_kernel_version(cc):
    """Return the (major, minor) version of the Linux kernel headers."""
    sym_data = ['#include <linux/version.h>', 'START',
                ('LINUX_VERSION_CODE', 'LINUX_VERSION_CODE')]
    val = glibcextract.compute_c_consts(sym_data, cc)['LINUX_VERSION_CODE']
    val = int(val)
    return ((val & 0xff0000) >> 16, (val & 0xff00) >> 8)

class ArchSyscall:
    """Canonical name and location of a syscall header."""

    def __init__(self, name, path):
        self.name = name
        self.path = path

    def __repr__(self):
        return 'ArchSyscall(name={!r}, patch={!r})'.format(
            self.name, self.path)

def list_arch_syscall_headers(topdir):
    """A generator which returns all the ArchSyscall objects in a tree."""

    sysdeps = os.path.join(topdir, 'sysdeps', 'unix', 'sysv', 'linux')
    for root, dirs, files in os.walk(sysdeps):
        if root != sysdeps:
            for filename in files:
                if filename == 'arch-syscall.h':
                    yield ArchSyscall(
                        name=os.path.relpath(root, sysdeps),
                        path=os.path.join(root, filename))

def list_syscall_uses(topdir):
    """Return a dictionary with the system calls issued by glibc itself.

    The values are the sorted lists of files, relative to TOPDIR, which
    issue the system call.  The sources are scanned textually.

    """
    uses = {}

    def add(name, path):
        uses.setdefault(name, set()).add(os.path.relpath(path, topdir))

    for root, dirs, files in os.walk(topdir):
        if '.git' in dirs:
            dirs.remove('.git')
        if 'config.make' in files:
            # A build directory: its generated headers list all numbers.
            del dirs[:]
            continue
        for filename in files:
            path = os.path.join(root, filename)

            if filename == 'syscalls.list':
                with open(path) as inp:
                    for line in inp:
                        comps = line.split()
                        # The third column is the system call name.
                        if len(comps) > 2 and not comps[0].startswith('#'):
                            add(comps[2], path)
                continue

            if filename in SYSCALL_NUMBER_HEADERS:
                continue
            if not filename.endswith(('.c', '.h', '.S')):
                continue
            with open(path, errors='replace') as inp:
                contents = inp.read()
            for match in RE_SYSCALL_NUMBER.finditer(contents):
                add(match.group(1), path)
            for match in RE_SYSCALL_MACRO.finditer(contents):
                add(match.group(1), path)
            if filename.endswith('.S'):
                for match in RE_SYSCALL_PSEUDO.finditer(contents):
                    add(match.group(1), path)

    return {name: sorted(paths) for name, paths in uses.items()}

def list_exported_symbols(topdir):
    """Return the symbols exported by the Linux ABIs of glibc.

    The symbols are read from the *.abilist files.

    """
    result = set()
    sysdeps = os.path.join(topdir, 'sysdeps', 'unix', 'sysv', 'linux')
    for root, _, files in os.walk(sysdeps):
        for filename in files:
            if not filename.endswith('.abilist'):
                continue
            with open(os.path.join(root, filename)) as inp:
                for line in inp:
                    # Lines have the form "GLIBC_2.0 symbol F".
                    comps = line.split()
                    if len(comps) > 1:
                        result.add(comps[1])
    return result

def legacy_syscalls(tables):
    """Return the legacy and architecture-private system calls in TABLES.

    These are the system calls which no architecture using the generic
    kernel system call ABI provides, plus LEGACY_SYSCALLS.

    """
    generic = set()
    for table in tables:
        if 'open' not in table.numbers:
            generic.update(table.numbers)
    all_syscalls = set(nr for table in tables for nr in table.numbers)
    return (all_syscalls - generic) | LEGACY_SYSCALLS

def __main():
    """Entry point when called as the main program."""

    import argparse
    import sys

    # Top-level directory of the source tree.
    topdir = os.path.realpath(os.path.join(
        os.path.dirname(os.path.realpath(__file__)), *('..',) * 4))

    def get_parser():
        parser = argparse.ArgumentParser(description=__doc__)
        subparsers = parser.add_subparsers(dest='command', required=True)
        subparsers.add_parser('list-headers',
            help='Print the absolute paths of all arch-syscall.h header files')
        subparser = subparsers.add_parser('query-syscall',
            help='Summarize the implementation status of system calls')
        subparser.add_argument('syscalls', help='Which syscalls to check',
                               nargs='+')
        subparser = subparsers.add_parser('list-missing-wrappers',
            help='List system calls without a glibc wrapper function')
        subparser.add_argument('--arch', action='append', metavar='NAME',
            help='Only list the system calls of this architecture (the'
            ' directory name below sysdeps/unix/sysv/linux); can be'
            ' repeated')
        subparser.add_argument('--internal', action='store_true',
            help='Also list the system calls which glibc issues without'
            ' exporting a wrapper function of the same name')
        subparser.add_argument('--legacy', action='store_true',
            help='Also list legacy and architecture-private system calls')
        subparser.add_argument('--verbose', action='store_true',
            help='Show the files which issue each listed system call')
        return parser
    parser = get_parser()
    args = parser.parse_args()

    if args.command == 'list-headers':
        for header in sorted([syscall.path for syscall
                              in list_arch_syscall_headers(topdir)]):
            print(header)

    elif args.command == 'query-syscall':
        # List of system call tables.
        tables = sorted(list_arch_syscall_headers(topdir),
                          key=lambda syscall: syscall.name)
        for table in tables:
            table.numbers = load_arch_syscall_header(table.path)

        for nr in args.syscalls:
            defined = [table.name for table in tables
                           if nr in table.numbers]
            undefined = [table.name for table in tables
                             if nr not in table.numbers]
            if not defined:
                print('{}: not defined on any architecture'.format(nr))
            elif not undefined:
                print('{}: defined on all architectures'.format(nr))
            else:
                print('{}:'.format(nr))
                print('  defined: {}'.format(' '.join(defined)))
                print('  undefined: {}'.format(' '.join(undefined)))

    elif args.command == 'list-missing-wrappers':
        tables = sorted(list_arch_syscall_headers(topdir),
                        key=lambda syscall: syscall.name)
        for table in tables:
            table.numbers = load_arch_syscall_header(table.path)

        # Legacy system calls are determined from all architectures, so
        # that the result does not depend on the --arch selection.
        skip = set() if args.legacy else legacy_syscalls(tables)

        if args.arch:
            known = set(table.name for table in tables)
            unknown = sorted(set(args.arch) - known)
            if unknown:
                parser.error('unknown architecture(s): {}\nknown: {}'.format(
                    ' '.join(unknown), ' '.join(sorted(known))))
            tables = [table for table in tables if table.name in args.arch]

        uses = list_syscall_uses(topdir)
        symbols = list_exported_symbols(topdir)

        print('# {:<26} {:<9} {}'.format(
            'system call', 'status',
            '' if len(tables) == 1 else 'architectures').rstrip())
        print('# status: unused = not issued by glibc,'
              ' internal = issued but not exported')

        for nr in sorted(set(nr for table in tables for nr in table.numbers)):
            if nr in skip:
                continue
            if nr in symbols or '__' + nr in symbols:
                # There is an exported function of the same name.
                continue
            used = uses.get(nr)
            if used and not args.internal:
                continue
            if len(tables) == 1:
                arches = ''
            else:
                arches = [table.name for table in tables
                          if nr in table.numbers]
                arches = ('all' if len(arches) == len(tables)
                          else ' '.join(arches))
            print('{:<28} {:<9} {}'.format(
                nr, 'internal' if used else 'unused', arches).rstrip())
            if args.verbose and used:
                for path in used:
                    print('    {}'.format(path))

    else:
        # Unrecognized command.
        usage(1)

if __name__ == '__main__':
    __main()
