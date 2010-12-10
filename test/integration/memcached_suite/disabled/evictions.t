#!/usr/bin/perl
# Test the 'stats items' evictions counters.

use strict;
use Test::More tests => 92;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached("-m 3");
my $sock = $server->sock;
my $value = "B"x66560;
my $key = 0;

# These aren't set to expire.
for ($key = 0; $key < 40; $key++) {
    print $sock "set key$key 0 0 66560\r\n$value\r\n";
    is(scalar <$sock>, "STORED\r\n", "stored key$key");
}

# These ones would expire in 600 seconds.
for ($key = 0; $key < 50; $key++) {
    print $sock "set key$key 0 600 66560\r\n$value\r\n";
    is(scalar <$sock>, "STORED\r\n", "stored key$key");
}

my $stats  = mem_stats($sock, "items");
my $evicted = $stats->{"items:31:evicted"};
isnt($evicted, "0", "check evicted");
my $evicted_nonzero = $stats->{"items:31:evicted_nonzero"};
isnt($evicted_nonzero, "0", "check evicted_nonzero");
