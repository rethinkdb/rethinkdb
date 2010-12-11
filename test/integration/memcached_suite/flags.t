#!/usr/bin/perl

use strict;
use Test::More tests => 6;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;

# set foo (and should get it)
for my $flags (0, 123, 2**16-1) {
    print $sock "set foo $flags 0 6\r\nfooval\r\n";
    is(scalar <$sock>, "STORED\r\n", "stored foo");
    mem_get_is({ sock => $sock,
                 flags => $flags }, "foo", "fooval", "got flags $flags back");
}
