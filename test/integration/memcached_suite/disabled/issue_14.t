#!/usr/bin/perl

use strict;
use Test::More tests => 21;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;
my $value = "B"x66560;
my $key = 0;

for ($key = 0; $key < 10; $key++) {
    print $sock "set key$key 0 2 66560\r\n$value\r\n";
    is (scalar <$sock>, "STORED\r\n", "stored key$key");
}

#print $sock "stats slabs"
my $first_stats  = mem_stats($sock, "slabs");
my $first_malloc = $first_stats->{total_malloced};

sleep(4);

for ($key = 10; $key < 20; $key++) {
    print $sock "set key$key 0 2 66560\r\n$value\r\n";
    is (scalar <$sock>, "STORED\r\n", "stored key$key");
}

my $second_stats  = mem_stats($sock, "slabs");
my $second_malloc = $second_stats->{total_malloced};


is ($second_malloc, $first_malloc, "Memory grows..")
