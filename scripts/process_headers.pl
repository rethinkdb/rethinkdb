#!/usr/bin/perl

# process_headers.pl

# Dear reader of this code,
#
#     Let me give you my dearest apologies for writing this code in
# Perl.  I had thought to write a one-off script for processing header
# files and seeing which header files are used the most.  Instead, I
# created a Perl program that got larger, and larger, and larger...
#
# Semisincerely,
#     Sam


# Usage:
#
# First cd to the src directory.
#   $ cd rethinkdb/src
#
# Then run one of these:
#   $ ../scripts/process_headers.pl
#      # prints how many cc files includes each header, and what
#      # boosts each header uses
#
#   $ ../scripts/process_headers.pl ./path/to/header.hpp
#   $ ../scripts/process_headers.pl -x ./path/to/header.hpp
#
# The latter two commands produce subsets of the first command's
# output.  I think one lists headers included by the argument, and the
# other lists headers that include the argument.
#
# This program will report mutually recursive headers, except for
# mutually recursive pairs of .tcc and .hpp headers (which is
# tolerated because it's harmless and makes clang-complete work better
# for some developers).
#
# This program expects every include directive to have absolute paths
# (by which I mean relative paths from the base of the src/
# directory).
#
# The command line arguments must begin with "./" as shown above,
# there is only one valid way to write them.

use strict;
use File::Find;

my @paths;

my %edges;
my %boosts;

sub callback {
    my $path = $File::Find::name;

    if ($path !~ /\.(cc|hpp|tcc)/) {
        return;
    }

    push @paths, $path;
}

find(\&callback, ".");

for my $path (@paths) {

    open(my $FH, '<', $path) or die "could not open $path\n";

    $edges{$path} = {};
    $boosts{$path} = {};

    while (<$FH>) {
        if (/^#include\s+"(.*)"/) {
            my $header = $1;
            $header =~ s!^!./!;
            $header =~ s!^\./\./!./!;
            if ($header =~ /\.\./) {
                die "weird $path $header\n";
            }

            if ($header !~ /\.pb\.h$/) {
                open(my $HFH, '<', $header)
                    or die "header file $header seems not to exist (from within $path)!\n";
                close($HFH);
            }

            $edges{$path}->{$header} = 1;
        } elsif (/^#include\s+<(boost\/.*)>/) {
            my $header = $1;
            $boosts{$path}->{$header} = 1;
        }
    }

    close($FH);
}

sub traverse ($$$) {
    my ($path, $parents, $visited) = @_;

    $visited->{$path} = 1;

    push @$parents, $path;
    for my $header (keys %{$edges{$path}}) {
        if (grep { $_ eq $header } @$parents) {
            my $tmp = $header;
            my $prev = $parents->[-1];
            my $untrunc_prev;

            $tmp =~ s/\.(tcc|hpp)$/\./;
            if ($1 eq 'tcc') {
                $prev =~ s/\.hpp$/\./;
            } else {
                $prev =~ s/\.tcc$/\./;
            }

            if ($tmp ne $prev) {
                print "header = $header, tmp = $tmp, prev = $prev\n";
                for my $parent (@$parents) {
                    print "$parent", ", ";
                }
                print "$header\n";
                die "Recursive include!\n";
            }
        }

        if (!$visited->{$header}) {
            traverse($header, $parents, $visited);
        }
    }

    pop @$parents;

}

{
    my $visited = {};
    for my $path (keys %edges) {
        traverse($path, [], $visited);
    }
}

sub squash () {
    my $ret = 0;
    for my $path (keys %edges) {
        for my $header (keys %{$edges{$path}}) {
            if ($path ne $header) {
                for my $subh (keys %{$edges{$header}}) {
                    if (!$edges{$path}->{$subh}) {
                        $ret = 1;
                        $edges{$path}->{$subh} = 1;
                        # print ("Setting $path -> $subh to 1\n");
                    }
                }
            }
        }
    }

    return $ret;
}

while (squash()) { }

my %counts;

for my $path (keys %edges) {
    if ($path =~ /\.cc/) {
        for my $header (keys %{$edges{$path}}) {
            $counts{$header} ++;
        }
    }
}

sub includes_header ($$) {
    my ($header, $val) = @_;
    # print("val: $val, header: $header, ", $edges{$val}->{$header}, "\n");
    return $edges{$val}->{$header};
}

my @pairs;
if ($ARGV[0] && $ARGV[0] eq '-x') {
    @pairs = sort { $a->[1] <=> $b->[1] } map { [$_, $counts{$_}] } (keys %{$edges{$ARGV[1]}});

} elsif ($ARGV[0]) {
    @pairs = sort { $a->[1] <=> $b->[1] } map { [$_, $counts{$_}] } grep { includes_header($ARGV[0], $_) } keys %counts;

} else {

    @pairs = sort { $a->[1] <=> $b->[1] } map { [$_, $counts{$_}] } keys %counts;

}

for my $pair (@pairs) {
    my $header = $pair->[0];
    my $value = $pair->[1];
    print "$value : $header";
    my $spacing = "    ";
    for my $boost (keys %{$boosts{$header}}) {
        print "$spacing<$boost>";
        $spacing = " ";
    }

    print "\n";
}
