#!/usr/bin/perl
#

use strict;
use lib '../../api/perl/lib';
use Cache::Memcached;
use Time::HiRes qw(time);

unless (@ARGV == 2) {
    die "Usage: stress-memcached.pl ip:port threads\n";
}

my $host = shift;
my $threads = shift;

my $memc = new Cache::Memcached;
$memc->set_servers([$host]);

unless ($memc->set("foo", "bar") &&
        $memc->get("foo") eq "bar") {
    die "memcached not running at $host ?\n";
}
$memc->disconnect_all();


my $running = 0;
while (1) {
    if ($running < $threads) {
        my $cpid = fork();
        if ($cpid) {
            $running++;
            #print "Launched $cpid.  Running $running threads.\n";
        } else {
            stress();
            exit 0;
        }
    } else {
        wait();
        $running--;
    }
}

sub stress {
    undef $memc;
    $memc = new Cache::Memcached;
    $memc->set_servers([$host]);

    my ($t1, $t2);
    my $start = sub { $t1 = time(); };
    my $stop = sub {
        my $op = shift;
        $t2 = time();
        my $td = sprintf("%0.3f", $t2 - $t1);
        if ($td > 0.25) { print "Took $td seconds for: $op\n"; }
    };

    my $max = rand(50);
    my $sets = 0;

    for (my $i = 0; $i < $max; $i++) {
        my $key = key($i);
        my $set = $memc->set($key, $key);
        $sets++ if $set;
    }

    for (1..int(rand(500))) {
        my $rand = int(rand($max));
        my $key = key($rand);
        my $meth = int(rand(3));
        my $exp = int(rand(3));
        undef $exp unless $exp;
        $start->();
        if ($meth == 0) {
            $memc->add($key, $key, $exp);
            $stop->("add");
        } elsif ($meth == 1) {
            $memc->delete($key);
            $stop->("delete");
        } else {
            $memc->set($key, $key, $exp);
            $stop->("set");
        }
        $rand = int(rand($max));
        $key = key($rand);
        $start->();
        my $v = $memc->get($key);
        $stop->("get");
        if ($v && $v ne $key) { die "Bogus: $v for key $rand\n"; }
    }

    $start->();
    my $multi = $memc->get_multi(map { key(int(rand($max))) } (1..$max));
    $stop->("get_multi");
}

sub key {
    my $n = shift;
    $_ = sprintf("%04d", $n);
    if ($n % 2) { $_ .= "a"x20; }
    $_;
}
