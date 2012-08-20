#!/usr/bin/perl

use strict;
use Test::More tests => 13;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock  = $server->sock;
my $sock2 = $server->new_sock;

ok($sock != $sock2, "have two different connections open");

# set large value
my $size   = 256 * 1024;  # 256 kB
my $bigval = "0123456789abcdef" x ($size / 16);
$bigval =~ s/^0/\[/; $bigval =~ s/f$/\]/;
my $bigval2 = uc($bigval);

print $sock "set big 0 0 $size\r\n$bigval\r\n";
is(scalar <$sock>, "STORED\r\n", "stored foo");
mem_get_is($sock, "big", $bigval, "big value got correctly");

print $sock "get big\r\n";
my $buf;
is(read($sock, $buf, $size / 2), $size / 2, "read half the answer back");
like($buf, qr/VALUE big/, "buf has big value header in it");
like($buf, qr/abcdef/, "buf has some data in it");
unlike($buf, qr/abcde\]/, "buf doesn't yet close");

# sock2 interrupts (maybe sock1 is slow) and deletes stuff:
print $sock2 "delete big\r\n";
is(scalar <$sock2>, "DELETED\r\n", "deleted big from sock2 while sock1's still reading it");
mem_get_is($sock2, "big", undef, "nothing from sock2 now.  gone from namespace.");
print $sock2 "set big 0 0 $size\r\n$bigval2\r\n";
is(scalar <$sock2>, "STORED\r\n", "stored big w/ val2");
mem_get_is($sock2, "big", $bigval2, "big value2 got correctly");

# sock1 resumes reading...
$buf .= <$sock>;
$buf .= <$sock>;
like($buf, qr/abcde\]/, "buf now closes");

# and if sock1 reads again, it's the uppercase version:
mem_get_is($sock, "big", $bigval2, "big value2 got correctly from sock1");
