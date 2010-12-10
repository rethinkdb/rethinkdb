#!/usr/bin/perl

use strict;
use Test::More tests => 22;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;
use Carp qw(croak);

use Cwd;
my $builddir = getcwd;

$ENV{'MEMCACHED_PORT_FILENAME'} = "/tmp/ports.$$";

sub read_ports {
    my %rv = ();
    open(my $f, "/tmp/ports.$$") || die("Can't open ports file.");
    while(<$f>) {
        my ($type, $port) = split(/:\s+/);
        $rv{$type} = $port + 0;
    }
    unlink "/tmp/ports.$$";
    return %rv;
}

sub validate_port {
    my ($name, $got, $expected) = @_;
    # diag "Wanted $expected, got $got";
    if ($expected == -1) {
        ok(!defined($got), "$name expected no port, got $got");
    } elsif ($expected == 0) {
        ok($got != 11211, "$name expected random port (got $got)");
    } else {
        is($got, $expected, "$name");
    }
}

sub run_server {
    my ($args) = @_;

    my $exe = "$builddir/memcached-debug";
    croak("memcached binary doesn't exist.  Haven't run 'make' ?\n") unless -e $exe;

    my $childpid = fork();

    my $cmd = "$builddir/timedrun 10 $exe $args";

    unless($childpid) {
        exec $cmd;
        exit; # NOTREACHED
    }

    for (1..20) {
        if (-f "/tmp/ports.$$") {
            return Memcached::Handle->new(pid  => $childpid);
        }
        select undef, undef, undef, 0.10;
    }
    croak "Failed to start server.";
}

sub when {
    my ($name, $params, $expected_tcp, $expected_udp) = @_;

    my $server = run_server($params);
    my %ports = read_ports();

    validate_port($name, $ports{'TCP INET'}, $expected_tcp);
    validate_port($name, $ports{'UDP INET'}, $expected_udp);
}

# Disabling the defaults since it conflicts with a running instance.
# when('no arguments', '', 11211, 11211);
when('specifying tcp port', '-p 11212', 11212, 11212);
when('specifying udp port', '-U 11222', 11222, 11222);
when('specifying tcp ephemeral port', '-p -1', 0, 0);
when('specifying udp ephemeral port', '-U -1', 0, 0);
when('tcp port disabled', '-p 0', -1, -1);
when('udp port disabled', '-U 0', -1, -1);
when('specifying tcp and udp ports', '-p 11232 -U 11233', 11232, 11233);
when('specifying tcp and disabling udp', '-p 11242 -U 0', 11242, -1);
when('specifying udp and disabling tcp', '-p -1 -U 11252', 0, 11252);
when('specifying tcp and ephemeral udp', '-p 11262 -U -1', 11262, 0);
when('specifying udp and ephemeral tcp', '-p -1 -U 11272', 0, 11272);
