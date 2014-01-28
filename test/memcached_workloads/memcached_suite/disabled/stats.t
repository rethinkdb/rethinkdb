#!/usr/bin/perl

use strict;
use Test::More tests => 95;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
my $sock = $server->sock;


## Output looks like this:
##
## STAT pid 22969
## STAT uptime 13
## STAT time 1259170891
## STAT version 1.4.3
## STAT pointer_size 32
## STAT rusage_user 0.001198
## STAT rusage_system 0.003523
## STAT curr_connections 10
## STAT total_connections 11
## STAT connection_structures 11
## STAT cmd_get 0
## STAT cmd_set 0
## STAT cmd_flush 0
## STAT get_hits 0
## STAT get_misses 0
## STAT delete_misses 0
## STAT delete_hits 0
## STAT incr_misses 0
## STAT incr_hits 0
## STAT decr_misses 0
## STAT decr_hits 0
## STAT cas_misses 0
## STAT cas_hits 0
## STAT cas_badval 0
## STAT auth_cmds 0
## STAT auth_unknowns 0
## STAT bytes_read 7
## STAT bytes_written 0
## STAT limit_maxbytes 67108864
## STAT accepting_conns 1
## STAT listen_disabled_num 0
## STAT threads 4
## STAT conn_yields 0
## STAT bytes 0
## STAT curr_items 0
## STAT total_items 0
## STAT evictions 0
## STAT reclaimed 0

# note that auth stats are tested in auth specfic tests


my $stats = mem_stats($sock);

# Test number of keys
is(scalar(keys(%$stats)), 38, "38 stats values");

# Test initial state
foreach my $key (qw(curr_items total_items bytes cmd_get cmd_set get_hits evictions get_misses
                 bytes_written delete_hits delete_misses incr_hits incr_misses decr_hits
                 decr_misses listen_disabled_num)) {
    is($stats->{$key}, 0, "initial $key is zero");
}
is($stats->{accepting_conns}, 1, "initial accepting_conns is one");

# Do some operations

print $sock "set foo 0 0 6\r\nfooval\r\n";
is(scalar <$sock>, "STORED\r\n", "stored foo");
mem_get_is($sock, "foo", "fooval");

my $stats = mem_stats($sock);

foreach my $key (qw(total_items curr_items cmd_get cmd_set get_hits)) {
    is($stats->{$key}, 1, "after one set/one get $key is 1");
}

my $cache_dump = mem_stats($sock, " cachedump 1 100");
ok(defined $cache_dump->{'foo'}, "got foo from cachedump");

print $sock "delete foo\r\n";
is(scalar <$sock>, "DELETED\r\n", "deleted foo");

my $stats = mem_stats($sock);
is($stats->{delete_hits}, 1);
is($stats->{delete_misses}, 0);

print $sock "delete foo\r\n";
is(scalar <$sock>, "NOT_FOUND\r\n", "shouldn't delete foo again");

my $stats = mem_stats($sock);
is($stats->{delete_hits}, 1);
is($stats->{delete_misses}, 1);

# incr stats

sub check_incr_stats {
    my ($ih, $im, $dh, $dm) = @_;
    my $stats = mem_stats($sock);

    is($stats->{incr_hits}, $ih);
    is($stats->{incr_misses}, $im);
    is($stats->{decr_hits}, $dh);
    is($stats->{decr_misses}, $dm);
}

print $sock "incr i 1\r\n";
is(scalar <$sock>, "NOT_FOUND\r\n", "shouldn't incr a missing thing");
check_incr_stats(0, 1, 0, 0);

print $sock "decr d 1\r\n";
is(scalar <$sock>, "NOT_FOUND\r\n", "shouldn't decr a missing thing");
check_incr_stats(0, 1, 0, 1);

print $sock "set n 0 0 1\r\n0\r\n";
is(scalar <$sock>, "STORED\r\n", "stored n");

print $sock "incr n 3\r\n";
is(scalar <$sock>, "3\r\n", "incr works");
check_incr_stats(1, 1, 0, 1);

print $sock "decr n 1\r\n";
is(scalar <$sock>, "2\r\n", "decr works");
check_incr_stats(1, 1, 1, 1);

# cas stats

sub check_cas_stats {
    my ($ch, $cm, $cb) = @_;
    my $stats = mem_stats($sock);

    is($stats->{cas_hits}, $ch);
    is($stats->{cas_misses}, $cm);
    is($stats->{cas_badval}, $cb);
}

check_cas_stats(0, 0, 0);

print $sock "cas c 0 0 1 99999999\r\nz\r\n";
is(scalar <$sock>, "NOT_FOUND\r\n", "missed cas");
check_cas_stats(0, 1, 0);

print $sock "set c 0 0 1\r\nx\r\n";
is(scalar <$sock>, "STORED\r\n", "stored c");
my ($id, $v) = mem_gets($sock, 'c');
is('x', $v, 'got the expected value');

print $sock "cas c 0 0 1 99999999\r\nz\r\n";
is(scalar <$sock>, "EXISTS\r\n", "missed cas");
check_cas_stats(0, 1, 1);
my ($newid, $v) = mem_gets($sock, 'c');
is('x', $v, 'got the expected value');

print $sock "cas c 0 0 1 $id\r\nz\r\n";
is(scalar <$sock>, "STORED\r\n", "good cas");
check_cas_stats(1, 1, 1);
my ($newid, $v) = mem_gets($sock, 'c');
is('z', $v, 'got the expected value');

my $settings = mem_stats($sock, ' settings');
is(1024, $settings->{'maxconns'});
is('NULL', $settings->{'domain_socket'});
is('on', $settings->{'evictions'});
is('yes', $settings->{'cas_enabled'});
is('no', $settings->{'auth_enabled_sasl'});

print $sock "stats reset\r\n";
is(scalar <$sock>, "RESET\r\n", "good stats reset");

my $stats = mem_stats($sock);
is(0, $stats->{'cmd_get'});
is(0, $stats->{'cmd_set'});
is(0, $stats->{'get_hits'});
is(0, $stats->{'get_misses'});
is(0, $stats->{'delete_misses'});
is(0, $stats->{'delete_hits'});
is(0, $stats->{'incr_misses'});
is(0, $stats->{'incr_hits'});
is(0, $stats->{'decr_misses'});
is(0, $stats->{'decr_hits'});
is(0, $stats->{'cas_misses'});
is(0, $stats->{'cas_hits'});
is(0, $stats->{'cas_badval'});
is(0, $stats->{'evictions'});
is(0, $stats->{'reclaimed'});

print $sock "flush_all\r\n";
is(scalar <$sock>, "OK\r\n", "flushed");

my $stats = mem_stats($sock);
is($stats->{cmd_flush}, 1, "after one flush cmd_flush is 1");
