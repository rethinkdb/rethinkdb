#!/usr/bin/perl
use strict;
use FindBin qw($Bin);
our @files;

BEGIN {
    chdir "$Bin/.." or die;
    @files = ( "doc/protocol.txt" );
}

use Test::More tests => scalar(@files);

foreach my $f (@files) {
    open(my $fh, $f) or die("Can't open $f");
    my @long_lines = ();
    my $line_number = 0;
    while(<$fh>) {
        $line_number++;
        if(length($_) > 80) {
            push(@long_lines, $line_number);
        }
    }
    close($fh);
    ok(@long_lines == 0, "$f has a long lines:  @long_lines");
}
