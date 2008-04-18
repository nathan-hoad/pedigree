#!/usr/bin/perl

#
# Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

#
# Creates a file called Version.cc in `pwd`/src/system/kernel.
# This script accepts build flags in the same form as the compiler (-DFLAG).
#

use strict;
use warnings;

my @flags;
foreach (@ARGV) {
  push @flags, $_;
}

my @time = localtime(time);
my @months = qw/Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec/;
my @days = qw/Sunday Monday Tuesday Wednesday Thursday Friday Saturday/;

my $day = $days[$time[6]];
my $month = $months[$time[4]];
my $dayofmonth = $time[3];
my $hour = $time[2];
my $minute = $time[1];
my $year = $time[7] + 1900;

my $time = "$hour:$minute $day $dayofmonth-$month-$year";

my ($revision) = `svn info` =~ m/^Revision: (\d+)$/m;

my $flags = join ' ', @flags;

my $user = `whoami`;
chomp $user;

my $machine = `uname -n`;
chomp $machine;
$machine .= ' (' . `uname -m`;
chomp $machine;
$machine .= ')';

open my $w, ">", "./src/system/kernel/Version.cc";

print $w "/* Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin\n";
print $w " *\n";
print $w " * Permission to use, copy, modify, and distribute this software for any\n";
print $w " * purpose with or without fee is hereby granted, provided that the above\n";
print $w " * copyright notice and this permission notice appear in all copies.\n";
print $w " *\n";
print $w " * THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES\n";
print $w " * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF\n";
print $w " * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR\n";
print $w " * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES\n";
print $w " * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN\n";
print $w " * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF\n";
print $w " * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\n";
print $w " */\n";
print $w "\n";
print $w "/*\n";
print $w " * AUTOGENERATED BY 'scripts/createVersionCc.pl' -- DO NOT EDIT!\n";
print $w " *                                                  ===========\n";
print $w " */\n\n";

print $w "const char *g_pBuildTime = \"$time\";\n";
print $w "const char *g_pBuildRevision = \"$revision\";\n";
print $w "const char *g_pBuildFlags = \"$flags\";\n";
print $w "const char *g_pBuildUser = \"$user\";\n";
print $w "const char *g_pBuildMachine = \"$machine\";\n";

close $w;
