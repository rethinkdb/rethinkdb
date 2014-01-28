#!/usr/bin/perl

use strict;
use Test::More tests => 4;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;

print $sock "set issue70 0 0 0\r\n\r\n";
is (scalar <$sock>, "STORED\r\n", "stored issue70");

print $sock "set issue70 0 0 -1\r\n";
is (scalar <$sock>, "CLIENT_ERROR bad command line format\r\n");

print $sock "set issue70 0 0 4294967295\r\n";
is (scalar <$sock>, "CLIENT_ERROR bad command line format\r\n");

print $sock "set issue70 0 0 2147483647\r\nscoobyscoobydoo";
is (scalar <$sock>, "CLIENT_ERROR bad command line format\r\n");
