#!/usr/bin/perl

use strict;
use Test::More tests => 18;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

eval {
    my $server = new_memcached();
    ok($server, "started the server");
};
is($@, '', 'Basic startup works');

eval {
    my $server = new_memcached("-l fooble");
};
ok($@, "Died with illegal -l args");

eval {
    my $server = new_memcached("-l 127.0.0.1");
};
is($@,'', "-l 127.0.0.1 works");

eval {
    my $server = new_memcached('-C');
    my $stats = mem_stats($server->sock, 'settings');
    is('no', $stats->{'cas_enabled'});
};
is($@, '', "-C works");

eval {
    my $server = new_memcached('-b 8675');
    my $stats = mem_stats($server->sock, 'settings');
    is('8675', $stats->{'tcp_backlog'});
};
is($@, '', "-b works");

foreach my $val ('auto', 'ascii') {
    eval {
        my $server = new_memcached("-B $val");
        my $stats = mem_stats($server->sock, 'settings');
        ok($stats->{'binding_protocol'} =~ /$val/, "$val works");
    };
    is($@, '', "$val works");
}

# For the binary test, we just verify it starts since we don't have an easy bin client.
eval {
    my $server = new_memcached("-B binary");
};
is($@, '', "binary works");

eval {
    my $server = new_memcached("-vv -B auto");
};
is($@, '', "auto works");

eval {
    my $server = new_memcached("-vv -B ascii");
};
is($@, '', "ascii works");


# For the binary test, we just verify it starts since we don't have an easy bin client.
eval {
    my $server = new_memcached("-vv -B binary");
};
is($@, '', "binary works");


# Should blow up with something invalid.
eval {
    my $server = new_memcached("-B http");
};
ok($@, "Died with illegal -B arg.");

# Should not allow -t 0
eval {
    my $server = new_memcached("-t 0");
};
ok($@, "Died with illegal 0 thread count");
