#!/usr/bin/perl

use strict;
use Test::More tests => 149;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

# assuming max slab is 1M and default mem is 64M
my $server = new_memcached();
my $sock = $server->sock;

# create a big value for the largest slab
my $max = 1024 * 1024;
my $big = 'x' x (1024 * 1024 - 250);

ok(length($big) > 512 * 1024);
ok(length($big) < 1024 * 1024);

# test that an even bigger value is rejected while we're here
my $too_big = $big . $big . $big;
my $len = length($too_big);
print $sock "set too_big 0 0 $len\r\n$too_big\r\n";
is(scalar <$sock>, "SERVER_ERROR object too large for cache\r\n", "too_big not stored");

# set the big value
my $len = length($big);
print $sock "set big 0 0 $len\r\n$big\r\n";
is(scalar <$sock>, "STORED\r\n", "stored big");
mem_get_is($sock, "big", $big);

# no evictions yet
my $stats = mem_stats($sock);
is($stats->{"evictions"}, "0", "no evictions to start");

# set many big items, enough to get evictions
for (my $i = 0; $i < 100; $i++) {
  print $sock "set item_$i 0 0 $len\r\n$big\r\n";
  is(scalar <$sock>, "STORED\r\n", "stored item_$i");
}

# some evictions should have happened
my $stats = mem_stats($sock);
my $evictions = int($stats->{"evictions"});
ok($evictions == 37, "some evictions happened");

# the first big value should be gone
mem_get_is($sock, "big", undef);

# the earliest items should be gone too
for (my $i = 0; $i < $evictions - 1; $i++) {
  mem_get_is($sock, "item_$i", undef);
}

# check that the non-evicted are the right ones
for (my $i = $evictions - 1; $i < $evictions + 4; $i++) {
  mem_get_is($sock, "item_$i", $big);
}
