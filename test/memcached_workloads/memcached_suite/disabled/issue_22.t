#!/usr/bin/perl

use strict;
use Test::More tests => 84;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached("-m 3");
my $sock = $server->sock;
my $value = "B"x66560;
my $key = 0;

for ($key = 0; $key < 40; $key++) {
    print $sock "set key$key 0 0 66560\r\n$value\r\n";
    is (scalar <$sock>, "STORED\r\n", "stored key$key");
}

my $first_stats  = mem_stats($sock, "items");
my $first_evicted = $first_stats->{"items:31:evicted"};
# I get 1 eviction on a 32 bit binary, but 4 on a 64 binary..
# Just check that I have evictions...
isnt ($first_evicted, "0", "check evicted");

print $sock "stats reset\r\n";
is (scalar <$sock>, "RESET\r\n", "Stats reset");

my $second_stats  = mem_stats($sock, "items");
my $second_evicted = $second_stats->{"items:31:evicted"};
is ("0", $second_evicted, "check evicted");

for ($key = 40; $key < 80; $key++) {
    print $sock "set key$key 0 0 66560\r\n$value\r\n";
    is (scalar <$sock>, "STORED\r\n", "stored key$key");
}

my $last_stats  = mem_stats($sock, "items");
my $last_evicted = $last_stats->{"items:31:evicted"};
is ($last_evicted, "40", "check evicted");
