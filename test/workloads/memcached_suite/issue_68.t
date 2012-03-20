#!/usr/bin/perl

use strict;
use Test::More tests => 996;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;

for (my $keyi = 1; $keyi < 250; $keyi++) {
    my $key = "x" x $keyi;
    print $sock "set $key 0 0 1\r\n9\r\n";
    is (scalar <$sock>, "STORED\r\n", "stored $key");
    mem_get_is($sock, $key, "9");
    print $sock "incr $key 1\r\n";
    is (scalar <$sock>, "10\r\n", "incr $key to 10");
    mem_get_is($sock, $key, "10");
}

