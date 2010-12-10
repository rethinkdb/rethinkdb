#!/usr/bin/perl

use strict;
use Test::More tests => 15;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;
my $expire;

sub wait_for_early_second {
    my $have_hires = eval "use Time::HiRes (); 1";
    if ($have_hires) {
        my $tsh = Time::HiRes::time();
        my $ts = int($tsh);
        return if ($tsh - $ts) < 0.5;
    }

    my $ts = int(time());
    while (1) {
        my $t = int(time());
        return if $t != $ts;
        select undef, undef, undef, 0.10;  # 1/10th of a second sleeps until time changes.
    }
}

wait_for_early_second();

print $sock "set foo 0 1 6\r\nfooval\r\n";
is(scalar <$sock>, "STORED\r\n", "stored foo");

mem_get_is($sock, "foo", "fooval");
sleep(1.5);
mem_get_is($sock, "foo", undef);

$expire = time() - 1;
print $sock "set foo 0 $expire 6\r\nfooval\r\n";
is(scalar <$sock>, "STORED\r\n", "stored foo");
mem_get_is($sock, "foo", undef, "already expired");

$expire = time() + 1;
print $sock "set foo 0 $expire 6\r\nfoov+1\r\n";
is(scalar <$sock>, "STORED\r\n", "stored foo");
mem_get_is($sock, "foo", "foov+1");
sleep(2.2);
mem_get_is($sock, "foo", undef, "now expired");

$expire = time() - 20;
print $sock "set boo 0 $expire 6\r\nbooval\r\n";
is(scalar <$sock>, "STORED\r\n", "stored boo");
mem_get_is($sock, "boo", undef, "now expired");

print $sock "add add 0 2 6\r\naddval\r\n";
is(scalar <$sock>, "STORED\r\n", "stored add");
mem_get_is($sock, "add", "addval");
# second add fails
print $sock "add add 0 2 7\r\naddval2\r\n";
is(scalar <$sock>, "NOT_STORED\r\n", "add failure");
sleep(2.3);
print $sock "add add 0 2 7\r\naddval3\r\n";
is(scalar <$sock>, "STORED\r\n", "stored add again");
mem_get_is($sock, "add", "addval3");
