#!/usr/bin/perl

# process_timings_output.pl processes the output of `make TIMINGS=1`.
# Take the output of `make TIMINGS=1` (being sure to have -j1
# specified, no parallelization allowed here) and throw it through
# this script, and you'll get a sorted list of .cc files and how long
# it takes to build them.

use strict;

my $file = 0;

my @pairs;

while (<>) {
    if (/^\s*CC (.*) -o/) {
        $file = $1;
    } elsif (/^\d+inputs/) {
        # nothing
    } elsif (!$file) {
        # nothing
    } elsif (/^([0-9.]+)user/) {
        push @pairs, [$file, $1];
        $file = 0;
    }
}

my @sortedpairs = sort { $a->[1] <=> $b->[1] } @pairs;

for (@sortedpairs) {
    print $_->[1], " ", $_->[0], "\n";
}


