#!/usr/bin/perl

use strict;
use Test::More tests => 43;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;


my $server = new_memcached();
my $sock = $server->sock;
my $sock2 = $server->new_sock;

my @result;
my @result2;

ok($sock != $sock2, "have two different connections open");

sub check_args {
    my ($line, $name) = @_;

    my $svr = new_memcached();
    my $s = $svr->sock;

    print $s $line;
    is(scalar <$s>, "CLIENT_ERROR bad command line format\r\n", $name);
    undef $svr;
}

check_args "cas bad blah 0 0 0\r\n\r\n", "bad flags";
check_args "cas bad 0 blah 0 0\r\n\r\n", "bad exp";
check_args "cas bad 0 0 blah 0\r\n\r\n", "bad cas";
check_args "cas bad 0 0 0 blah\r\n\r\n", "bad size";

# gets foo (should not exist)
print $sock "gets foo\r\n";
is(scalar <$sock>, "END\r\n", "gets failed");

# set foo
print $sock "set foo 0 0 6\r\nbarval\r\n";
is(scalar <$sock>, "STORED\r\n", "stored barval");

# gets foo and verify identifier exists
@result = mem_gets($sock, "foo");
mem_gets_is($sock,$result[0],"foo","barval");

# cas fail
print $sock "cas foo 0 0 6 123\r\nbarva2\r\n";
is(scalar <$sock>, "EXISTS\r\n", "cas failed for foo");

# gets foo - success
@result = mem_gets($sock, "foo");
mem_gets_is($sock,$result[0],"foo","barval");

# cas success
print $sock "cas foo 0 0 6 $result[0]\r\nbarva2\r\n";
is(scalar <$sock>, "STORED\r\n", "cas success, set foo");

# cas failure (reusing the same key)
print $sock "cas foo 0 0 6 $result[0]\r\nbarva2\r\n";
is(scalar <$sock>, "EXISTS\r\n", "reusing a CAS ID");

# delete foo
print $sock "delete foo\r\n";
is(scalar <$sock>, "DELETED\r\n", "deleted foo");

# cas missing
print $sock "cas foo 0 0 6 $result[0]\r\nbarva2\r\n";
is(scalar <$sock>, "NOT_FOUND\r\n", "cas failed, foo does not exist");

# cas empty
print $sock "cas foo 0 0 6 \r\nbarva2\r\n";
is(scalar <$sock>, "ERROR\r\n", "cas empty, throw error");
# cant parse barval2\r\n
is(scalar <$sock>, "ERROR\r\n", "error out on barval2 parsing");

# set foo1
print $sock "set foo1 0 0 1\r\n1\r\n";
is(scalar <$sock>, "STORED\r\n", "set foo1");
# set foo2
print $sock "set foo2 0 0 1\r\n2\r\n";
is(scalar <$sock>, "STORED\r\n", "set foo2");

# gets foo1 check
print $sock "gets foo1\r\n";
ok(scalar <$sock> =~ /VALUE foo1 0 1 (\d+)\r\n/, "gets foo1 regexp success");
my $foo1_cas = $1;
is(scalar <$sock>, "1\r\n","gets foo1 data is 1");
is(scalar <$sock>, "END\r\n","gets foo1 END");

# gets foo2 check
print $sock "gets foo2\r\n";
ok(scalar <$sock> =~ /VALUE foo2 0 1 (\d+)\r\n/,"gets foo2 regexp success");
my $foo2_cas = $1;
is(scalar <$sock>, "2\r\n","gets foo2 data is 2");
is(scalar <$sock>, "END\r\n","gets foo2 END");

# validate foo1 != foo2
ok($foo1_cas != $foo2_cas,"foo1 != foo2 single-gets success");

# multi-gets
print $sock "gets foo1 foo2\r\n";
ok(scalar <$sock> =~ /VALUE foo1 0 1 (\d+)\r\n/, "validating first set of data is foo1");
$foo1_cas = $1;
is(scalar <$sock>, "1\r\n", "validating foo1 set of data is 1");
ok(scalar <$sock> =~ /VALUE foo2 0 1 (\d+)\r\n/, "validating second set of data is foo2");
$foo2_cas = $1;
is(scalar <$sock>, "2\r\n", "validating foo2 set of data is 2");
is(scalar <$sock>, "END\r\n","validating foo1,foo2 gets is over - END");

# validate foo1 != foo2
ok($foo1_cas != $foo2_cas, "foo1 != foo2 multi-gets success");

### simulate race condition with cas

# gets foo1 - success
@result = mem_gets($sock, "foo1");
ok($result[0] != "", "sock - gets foo1 is not empty");

# gets foo2 - success
@result2 = mem_gets($sock2, "foo1");
ok($result2[0] != "","sock2 - gets foo1 is not empty");

print $sock "cas foo1 0 0 6 $result[0]\r\nbarva2\r\n";
print $sock2 "cas foo1 0 0 5 $result2[0]\r\napple\r\n";

my $res1 = <$sock>;
my $res2 = <$sock2>;

ok( ( $res1 eq "STORED\r\n" && $res2 eq "EXISTS\r\n") ||
    ( $res1 eq "EXISTS\r\n" && $res2 eq "STORED\r\n"),
    "cas on same item from two sockets");

### bug 15: http://code.google.com/p/memcached/issues/detail?id=15

# set foo
print $sock "set bug15 0 0 1\r\n0\r\n";
is(scalar <$sock>, "STORED\r\n", "stored 0");

# Check out the first gets.
print $sock "gets bug15\r\n";
ok(scalar <$sock> =~ /VALUE bug15 0 1 (\d+)\r\n/, "gets bug15 regexp success");
my $bug15_cas = $1;
is(scalar <$sock>, "0\r\n", "gets bug15 data is 0");
is(scalar <$sock>, "END\r\n","gets bug15 END");

# Increment
print $sock "incr bug15 1\r\n";
is(scalar <$sock>, "1\r\n", "incr worked");

# Validate a changed CAS
print $sock "gets bug15\r\n";
ok(scalar <$sock> =~ /VALUE bug15 0 1 (\d+)\r\n/, "gets bug15 regexp success");
my $next_bug15_cas = $1;
is(scalar <$sock>, "1\r\n", "gets bug15 data is 0");
is(scalar <$sock>, "END\r\n","gets bug15 END");

ok($bug15_cas != $next_bug15_cas, "CAS changed");
