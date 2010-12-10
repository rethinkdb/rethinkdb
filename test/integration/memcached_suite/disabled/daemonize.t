#!/usr/bin/perl

use strict;
use Test::More tests => 7;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

use File::Temp qw(tempfile);

my (undef, $tmpfn) = tempfile();

my $server = new_memcached("-d -P $tmpfn");
my $sock = $server->sock;
sleep 0.5;

ok(-e $tmpfn, "pid file exists");
ok(-s $tmpfn, "pid file has length");

open (my $fh, $tmpfn) or die;
my $readpid = do { local $/; <$fh>; };
chomp $readpid;
close ($fh);

ok(kill(0, $readpid), "process is still running");

my $stats = mem_stats($sock);
is($stats->{pid}, $readpid, "memcached reports same pid as file");

ok($server->new_sock, "opened new socket");
ok(kill(9, $readpid), "sent KILL signal");
sleep 0.5;
ok(! $server->new_sock, "failed to open new socket");
