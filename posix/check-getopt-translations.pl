#! /usr/bin/perl

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

use strict;
use warnings;
use Data::Dumper;

my $VERSION = "@VERSION@";

my $PKGVERSION = "@PKGVERSION@";
my $REPORT_BUGS_TO = '@REPORT_BUGS_TO@';
my $progname = $_;

sub usage {
    print "Usage: getopt-check [OPTION]... msgctxt lang.po\n";
    print "  --help       print this help, then exit\n";
    print "  --version    print version number, then exit\n";
    print "\n";
    print "For bug reporting instructions, please see:\n";
    print "$REPORT_BUGS_TO.\n";
    exit 0;
}

sub fatal {
    print STDERR "$_[0]\n";
    exit 1;
}

# This script takes two positional arguments: the context for
# translated option names, and the PO file to check.  Then, the PO
# file is parsed, looking at three things:
# 1. The msgctxt: it must be equal to the first positional argument, msgctxt;
# 2. The msgid;
# 3. The space-separated list msgstr.
#
# We are looking for two different problems:
#
# 1. Every translation element, current or obsolete, must be unique
# across all option names.
# 2. For every option name, for every translation, current or
# deprecated, if it doesn’t match the untranslated name, then it
# should not match any other untranslated option names.
#
# If we detect an example of the first case, it is a problem with the
# translator only.  They have to remove one use of the word,
# preferably one that is deprecated.
#
# If we detect an example of the second case, then it is a problem
# with the developer: they want to introduce an option name that is
# already used for something else by users of this native language! If
# nothing is done, these users will be surprised that the same word
# now means another option, as the untranslated options have
# precedence over the translations.  If the translated name is already
# deprecated, then the language team may agree to completely remove
# it.  Otherwise, it may be better to find a new untranslated name.

 arglist: while (@ARGV) {
     if ($ARGV[0] eq "--v" || $ARGV[0] eq "--ve" || $ARGV[0] eq "--ver" ||
	$ARGV[0] eq "--vers" || $ARGV[0] eq "--versi" ||
	$ARGV[0] eq "--versio" || $ARGV[0] eq "--version") {
	print "getopt-check $PKGVERSION$VERSION\n";
	print "Copyright (C) 2026 Free Software Foundation, Inc.\n";
	print "This is free software; see the source for copying conditions.  There is NO\n";
	print "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n";
	print "Written by Vivien Kraus <vivien\@planete-kraus.eu>\n";

	exit 0;
    } elsif ($ARGV[0] eq "--h" || $ARGV[0] eq "--he" || $ARGV[0] eq "--hel" ||
	     $ARGV[0] eq "--help") {
	&usage;
    } elsif ($ARGV[0] =~ /^-/) {
	print "$progname: unrecognized option `$ARGV[0]'\n";
	print "Try `$progname --help' for more information.\n";
	exit 1;
    } else {
	last arglist;
    }
}

if ($#ARGV != 1) {
    fatal "You must provide two arguments: the msgctxt for option names, and the name of the PO file.";
}

my $relevant_msgctxt = $ARGV[0];
my $pofilename = $ARGV[1];
my %translations;

# %translation_used will be populated to detect multiple use of a
# %translation directly when we parse.

my $entry_msgid;

# The ad-hoc PO file parser has 3 states:
# 1. Waiting for msgctxt;
# 2. Waiting for msgid;
# 3. Waiting for msgstr.
#
# At the start, the state is 1.  Then, if we find "msgctxt
# \"$relevant_msgctxt\"" in a single line, we jump to 2.  Otherwise,
# if this is the end of the file, stop parsing.  Otherwise, whatever
# the line, stay in 1.  This includes: the empty line, meaning we are
# considering a new entry; or a comment, a #: location, or another
# relevant line.
#
# When we are in state 2., we are waiting for the msgid (untranslated
# option name).  If we find an empty line, we jump back to 1.  If we
# find a line starting with "msgid \"" and ending with a double quote,
# we store what is in the middle in $entry_msgid and jump to 3.
# Otherwise, we stay in state 2.
#
# When we are in state 3., we are waiting for msgstr.  If we find an
# empty line, drop $entry_msgid, and back to 1.  If the line starts
# with "msgstr \"", we add a record to %translations: the key is
# $entry_msgid, and the value, what is between the detected prefix and
# the end quote.  Then, back to state 1.

my $parser_state = 1;

open (my $pofile, "<", $pofilename) || fatal "PO file name ${pofilename} cannot be read.";

while (my $line = <$pofile>) {
    chomp $line;
    if ($parser_state == 1 && $line =~ /^msgctxt\s*"${relevant_msgctxt}"$/) {
        $parser_state = 2;
    } elsif ($parser_state == 2 && $line eq "") {
        $parser_state = 1;
    } elsif ($parser_state == 2 && $line =~ /^msgid\s*"([^"]+)"$/) {
        $parser_state = 3;
        $entry_msgid = $1;
    } elsif ($parser_state == 3 && $line eq "") {
        $parser_state = 1;
    } elsif ($parser_state == 3 && $line =~ /^msgstr\s*"([^"]*)"$/) {
        my @translations_for_this = split(/\s+/, $1);
        $translations{$entry_msgid} = \@translations_for_this;
        $parser_state = 1;
    }
}

my $number_of_errors = 0;

# Verify that every option name is unique.
my %untranslated_name;
for my $option_name (sort(keys %translations)) {
    for my $translation (@{$translations{$option_name}}) {
        my @existing;
        if (exists $untranslated_name{$translation}) {
            @existing = @{$untranslated_name{$translation}};
        }
        push(@existing, $option_name);
        $untranslated_name{$translation} = \@existing;
    }
}
for my $translation (sort(keys %untranslated_name)) {
    my $names = $untranslated_name{$translation};
    if (@{$names} > 1) {
        print STDERR "Translation ${translation} is used for more than one option:\n";
        for my $untranslated (@{$names}) {
            print STDERR "  - ${untranslated}\n";
        }
        ++$number_of_errors;
    }
}

# Verify that every option translation does not match any other
# untranslated name.
for my $option_name (sort(keys %translations)) {
    for my $other_option_name (sort(keys %translations)) {
        if ($option_name ne $other_option_name) {
            for my $translation (@{$translations{$option_name}}) {
                if ($translation eq $other_option_name) {
                    print STDERR "${translation} is a translation of ${option_name}, but it is also a different option.\n";
                    ++$number_of_errors;
                }
            }
        }
    }
}

if ($number_of_errors eq 0) {
    exit 0
}
print STDERR "There were ${number_of_errors} failures.\n";
exit 1
