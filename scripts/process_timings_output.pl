#!/usr/bin/perl
# Copyright 2010-2012 RethinkDB, all rights reserved.

# process_timings_output.pl processes the output of `make TIMINGS=1`.
# Take the output of `make TIMINGS=1` (being sure to have -j1
# specified, no parallelization allowed here) and throw it through
# this script, and you'll get a sorted list of .cc files and how long
# it takes to build them.

use strict;

my $file = 0;
my $sum = 0;

my @pairs;

while (<>) {
    if (/^\s*(\[\d+\/\d+\].*)$/) {
        push @pairs, [$file, $sum];
        $file = $1;
        $sum = 0;
    } elsif (/^user\s*(\d+)m(\d+\.\d+)s/) {
        $sum += $1 * 60 + $2;
    }
}

my @sortedpairs = sort { $a->[1] <=> $b->[1] } @pairs;

for (@sortedpairs) {
    print $_->[1], " ", $_->[0], "\n";
}


