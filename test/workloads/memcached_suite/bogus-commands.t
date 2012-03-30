#!/usr/bin/perl

use strict;
use Test::More tests => 1;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;

print $sock "boguscommand slkdsldkfjsd\r\n";
is(scalar <$sock>, "ERROR\r\n", "got error back");
