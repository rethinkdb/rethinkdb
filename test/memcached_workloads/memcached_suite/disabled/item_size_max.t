#!/usr/bin/perl

use strict;
use Test::More tests => 7;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;

my $stats = mem_stats($sock, ' settings');

# Ensure default still works.
is($stats->{item_size_max}, 1024 * 1024);
$server->stop();

# Should die.
eval {
    $server = new_memcached('-I 1000');
};
ok($@ && $@ =~ m/^Failed/, "Shouldn't start with < 1k item max");

eval {
    $server = new_memcached('-I 256m');
};
ok($@ && $@ =~ m/^Failed/, "Shouldn't start with > 128m item max");

# Minimum.
$server = new_memcached('-I 1024');
my $stats = mem_stats($server->sock, ' settings');
is($stats->{item_size_max}, 1024);
$server->stop();

# Reasonable but unreasonable.
$server = new_memcached('-I 1049600');
my $stats = mem_stats($server->sock, ' settings');
is($stats->{item_size_max}, 1049600);
$server->stop();

# Suffix kilobytes.
$server = new_memcached('-I 512k');
my $stats = mem_stats($server->sock, ' settings');
is($stats->{item_size_max}, 524288);
$server->stop();

# Suffix megabytes.
$server = new_memcached('-I 32m');
my $stats = mem_stats($server->sock, ' settings');
is($stats->{item_size_max}, 33554432);
$server->stop();

