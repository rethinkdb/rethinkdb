#!/usr/bin/perl

use strict;
use Test::More;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

$ENV{T_MEMD_INITIAL_MALLOC} = "4294967328"; # 2**32 + 32 , just over 4GB
$ENV{T_MEMD_SLABS_ALLOC}    = 0;  # don't preallocate slabs

my $server = new_memcached("-m 4098 -M");
my $sock = $server->sock;

my ($stats, $slabs) = @_;

$stats = mem_stats($sock);

if ($stats->{'pointer_size'} eq "32") {
    plan skip_all => 'Skipping 64-bit tests on 32-bit build';
    exit 0;
} else {
    plan tests => 6;
}

is($stats->{'pointer_size'}, 64, "is 64 bit");
is($stats->{'limit_maxbytes'}, "4297064448", "max bytes is 4098 MB");

$slabs = mem_stats($sock, 'slabs');
is($slabs->{'total_malloced'}, "4294967328", "expected (faked) value of total_malloced");
is($slabs->{'active_slabs'}, 0, "no active slabs");

my $hit_limit = 0;
for (1..5) {
    my $size = 400 * 1024;
    my $data = "a" x $size;
    print $sock "set big$_ 0 0 $size\r\n$data\r\n";
    my $res = <$sock>;
    $hit_limit = 1 if $res ne "STORED\r\n";
}
ok($hit_limit, "hit size limit");

$slabs = mem_stats($sock, 'slabs');
is($slabs->{'active_slabs'}, 1, "1 active slab");
