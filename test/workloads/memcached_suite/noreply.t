#!/usr/bin/perl

use strict;
use Test::More tests => 9;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;


my $server = new_memcached();
my $sock = $server->sock;


# Test that commands can take 'noreply' parameter.

# These tests aren't relevant to RethinkDB:
#print $sock "flush_all noreply\r\n";
#print $sock "flush_all 0 noreply\r\n";

#print $sock "verbosity 0 noreply\r\n";

print $sock "add noreply:foo 0 0 1 noreply\r\n1\r\n";
mem_get_is($sock, "noreply:foo", "1");

print $sock "set noreply:foo 0 0 1 noreply\r\n2\r\n";
mem_get_is($sock, "noreply:foo", "2");

print $sock "replace noreply:foo 0 0 1 noreply\r\n3\r\n";
mem_get_is($sock, "noreply:foo", "3");

print $sock "append noreply:foo 0 0 1 noreply\r\n4\r\n";
mem_get_is($sock, "noreply:foo", "34");

print $sock "prepend noreply:foo 0 0 1 noreply\r\n5\r\n";
my @result = mem_gets($sock, "noreply:foo");
ok($result[1] eq "534");

print $sock "cas noreply:foo 0 0 1 $result[0] noreply\r\n6\r\n";
mem_get_is($sock, "noreply:foo", "6");

print $sock "incr noreply:foo 3 noreply\r\n";
mem_get_is($sock, "noreply:foo", "9");

print $sock "decr noreply:foo 2 noreply\r\n";
mem_get_is($sock, "noreply:foo", "7");

print $sock "delete noreply:foo noreply\r\n";
mem_get_is($sock, "noreply:foo");

