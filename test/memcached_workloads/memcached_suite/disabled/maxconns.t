#!/usr/bin/perl

use strict;
use warnings;

use Test::More tests => 11;

use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;


# start up a server with 10 maximum connections
my $server = new_memcached('-c 10');
my $sock = $server->sock;
my @sockets;

ok(defined($sock), 'Connection 0');
push (@sockets, $sock);


foreach my $conn (1..10) {
  $sock = $server->new_sock;
  ok(defined($sock), "Made connection $conn");
  push(@sockets, $sock);
}
