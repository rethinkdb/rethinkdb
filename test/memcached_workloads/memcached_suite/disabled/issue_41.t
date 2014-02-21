#!/usr/bin/perl

use strict;
use warnings;
use POSIX qw(ceil);
use Test::More tests => 691;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;

my $factor = 2;
my $val = "x" x $factor;
my $key = '';

# SET items of diverse size to the daemon so it can attempt
# to return a large stats output for slabs
for (my $i=0; $i<69; $i++) {
    for (my $j=0; $j<10; $j++) {
        $key = "$i:$j";
        print $sock "set key$key 0 0 $factor\r\n$val\r\n";
        is (scalar <$sock>, "STORED\r\n", "stored key$key");
    }
    $factor *= 1.2;
    $factor = ceil($factor);
    $val = "x" x $factor;
}

# This request will kill the daemon if it has not allocated
# enough memory internally.
my $stats = mem_stats($sock, "slabs");

# Verify whether the daemon is still running or not by asking
# it for statistics.
print $sock "version\r\n";
my $v = scalar <$sock>;
ok(defined $v && length($v), "memcached didn't respond");
