#!/usr/bin/perl

use strict;
use Test::More tests => 24;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;
my $expire;

print $sock "stats detail dump\r\n";
is(scalar <$sock>, "END\r\n", "verified empty stats at start");

print $sock "stats detail on\r\n";
is(scalar <$sock>, "OK\r\n", "detail collection turned on");

print $sock "set foo:123 0 0 6\r\nfooval\r\n";
is(scalar <$sock>, "STORED\r\n", "stored foo");

print $sock "stats detail dump\r\n";
is(scalar <$sock>, "PREFIX foo get 0 hit 0 set 1 del 0\r\n", "details after set");
is(scalar <$sock>, "END\r\n", "end of details");

mem_get_is($sock, "foo:123", "fooval");
print $sock "stats detail dump\r\n";
is(scalar <$sock>, "PREFIX foo get 1 hit 1 set 1 del 0\r\n", "details after get with hit");
is(scalar <$sock>, "END\r\n", "end of details");

mem_get_is($sock, "foo:124", undef);

print $sock "stats detail dump\r\n";
is(scalar <$sock>, "PREFIX foo get 2 hit 1 set 1 del 0\r\n", "details after get without hit");
is(scalar <$sock>, "END\r\n", "end of details");

print $sock "delete foo:125\r\n";
is(scalar <$sock>, "NOT_FOUND\r\n", "sent delete command");

print $sock "stats detail dump\r\n";
is(scalar <$sock>, "PREFIX foo get 2 hit 1 set 1 del 1\r\n", "details after delete");
is(scalar <$sock>, "END\r\n", "end of details");

print $sock "stats reset\r\n";
is(scalar <$sock>, "RESET\r\n", "stats cleared");

print $sock "stats detail dump\r\n";
is(scalar <$sock>, "END\r\n", "empty stats after clear");

mem_get_is($sock, "foo:123", "fooval");
print $sock "stats detail dump\r\n";
is(scalar <$sock>, "PREFIX foo get 1 hit 1 set 0 del 0\r\n", "details after clear and get");
is(scalar <$sock>, "END\r\n", "end of details");

print $sock "stats detail off\r\n";
is(scalar <$sock>, "OK\r\n", "detail collection turned off");

mem_get_is($sock, "foo:124", undef);

mem_get_is($sock, "foo:123", "fooval");
print $sock "stats detail dump\r\n";
is(scalar <$sock>, "PREFIX foo get 1 hit 1 set 0 del 0\r\n", "details after stats turned off");
is(scalar <$sock>, "END\r\n", "end of details");
