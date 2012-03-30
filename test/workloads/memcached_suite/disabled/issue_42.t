#!/usr/bin/perl

use strict;
use Test::More tests => 11;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;
my $value = "B"x10;
my $key = 0;

for ($key = 0; $key < 10; $key++) {
    print $sock "set key$key 0 0 10\r\n$value\r\n";
    is (scalar <$sock>, "STORED\r\n", "stored key$key");
}

my $first_stats = mem_stats($sock, "slabs");
my $req = $first_stats->{"1:mem_requested"};
ok ($req == "640" || $req == "800", "Check allocated size");
