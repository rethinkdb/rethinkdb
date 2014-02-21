#!/usr/bin/perl

use strict;
use Test::More tests => 8;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;

my $count = 1;

foreach my $blob ("mooo\0", "mumble\0\0\0\0\r\rblarg", "\0", "\r") {
    my $key = "foo$count";
    my $len = length($blob);
    print "len is $len\n";
    print $sock "set $key 0 0 $len\r\n$blob\r\n";
    is(scalar <$sock>, "STORED\r\n", "stored $key");
    mem_get_is($sock, $key, $blob);
    $count++;
}

