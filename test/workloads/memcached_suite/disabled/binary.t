#!/usr/bin/perl

use strict;
use warnings;
use Test::More tests => 3361;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $server = new_memcached();
ok($server, "started the server");

# Based almost 100% off testClient.py which is:
# Copyright (c) 2007  Dustin Sallings <dustin@spy.net>

# Command constants
use constant CMD_GET        => 0x00;
use constant CMD_SET        => 0x01;
use constant CMD_ADD        => 0x02;
use constant CMD_REPLACE    => 0x03;
use constant CMD_DELETE     => 0x04;
use constant CMD_INCR       => 0x05;
use constant CMD_DECR       => 0x06;
use constant CMD_QUIT       => 0x07;
use constant CMD_FLUSH      => 0x08;
use constant CMD_GETQ       => 0x09;
use constant CMD_NOOP       => 0x0A;
use constant CMD_VERSION    => 0x0B;
use constant CMD_GETK       => 0x0C;
use constant CMD_GETKQ      => 0x0D;
use constant CMD_APPEND     => 0x0E;
use constant CMD_PREPEND    => 0x0F;
use constant CMD_STAT       => 0x10;
use constant CMD_SETQ       => 0x11;
use constant CMD_ADDQ       => 0x12;
use constant CMD_REPLACEQ   => 0x13;
use constant CMD_DELETEQ    => 0x14;
use constant CMD_INCREMENTQ => 0x15;
use constant CMD_DECREMENTQ => 0x16;
use constant CMD_QUITQ      => 0x17;
use constant CMD_FLUSHQ     => 0x18;
use constant CMD_APPENDQ    => 0x19;
use constant CMD_PREPENDQ   => 0x1A;

# REQ and RES formats are divided even though they currently share
# the same format, since they _could_ differ in the future.
use constant REQ_PKT_FMT      => "CCnCCnNNNN";
use constant RES_PKT_FMT      => "CCnCCnNNNN";
use constant INCRDECR_PKT_FMT => "NNNNN";
use constant MIN_RECV_BYTES   => length(pack(RES_PKT_FMT));
use constant REQ_MAGIC        => 0x80;
use constant RES_MAGIC        => 0x81;

my $mc = MC::Client->new;

# Let's turn on detail stats for all this stuff

$mc->stats('detail on');

my $check = sub {
    my ($key, $orig_flags, $orig_val) = @_;
    my ($flags, $val, $cas) = $mc->get($key);
    is($flags, $orig_flags, "Flags is set properly");
    ok($val eq $orig_val || $val == $orig_val, $val . " = " . $orig_val);
};

my $set = sub {
    my ($key, $exp, $orig_flags, $orig_value) = @_;
    $mc->set($key, $orig_value, $orig_flags, $exp);
    $check->($key, $orig_flags, $orig_value);
};

my $empty = sub {
    my $key = shift;
    my $rv =()= eval { $mc->get($key) };
    is($rv, 0, "Didn't get a result from get");
    ok($@->not_found, "We got a not found error when we expected one");
};

my $delete = sub {
    my ($key, $when) = @_;
    $mc->delete($key, $when);
    $empty->($key);
};

# diag "Test Version";
my $v = $mc->version;
ok(defined $v && length($v), "Proper version: $v");

# Bug 71
{
    my %stats1 = $mc->stats('');
    $mc->flush;
    my %stats2 = $mc->stats('');

    is($stats2{'cmd_flush'}, $stats1{'cmd_flush'} + 1,
       "Stats not updated on a binary flush");
}

# diag "Flushing...";
$mc->flush;

# diag "Noop";
$mc->noop;

# diag "Simple set/get";
$set->('x', 5, 19, "somevalue");

# diag "Delete";
$delete->('x');

# diag "Flush";
$set->('x', 5, 19, "somevaluex");
$set->('y', 5, 17, "somevaluey");
$mc->flush;
$empty->('x');
$empty->('y');

{
    # diag "Add";
    $empty->('i');
    $mc->add('i', 'ex', 5, 10);
    $check->('i', 5, "ex");

    my $rv =()= eval { $mc->add('i', "ex2", 10, 5) };
    is($rv, 0, "Add didn't return anything");
    ok($@->exists, "Expected exists error received");
    $check->('i', 5, "ex");
}

{
    # diag "Too big.";
    $empty->('toobig');
    $mc->set('toobig', 'not too big', 10, 10);
    eval {
        my $bigval = ("x" x (1024*1024)) . "x";
        $mc->set('toobig', $bigval, 10, 10);
    };
    ok($@->too_big, "Was too big");
    $empty->('toobig');
}

{
    # diag "Replace";
    $empty->('j');

    my $rv =()= eval { $mc->replace('j', "ex", 19, 5) };
    is($rv, 0, "Replace didn't return anything");
    ok($@->not_found, "Expected not_found error received");
    $empty->('j');
    $mc->add('j', "ex2", 14, 5);
    $check->('j', 14, "ex2");
    $mc->replace('j', "ex3", 24, 5);
    $check->('j', 24, "ex3");
}

{
    # diag "MultiGet";
    $mc->add('xx', "ex", 1, 5);
    $mc->add('wye', "why", 2, 5);
    my $rv = $mc->get_multi(qw(xx wye zed));

    # CAS is returned with all gets.
    $rv->{xx}->[2]  = 0;
    $rv->{wye}->[2] = 0;
    is_deeply($rv->{xx}, [1, 'ex', 0], "X is correct");
    is_deeply($rv->{wye}, [2, 'why', 0], "Y is correct");
    is(keys(%$rv), 2, "Got only two answers like we expect");
}

# diag "Test increment";
$mc->flush;
is($mc->incr("x"), 0, "First incr call is zero");
is($mc->incr("x"), 1, "Second incr call is one");
is($mc->incr("x", 211), 212, "Adding 211 gives you 212");
is($mc->incr("x", 2**33), 8589934804, "Blast the 32bit border");

# diag "Issue 48 - incrementing plain text.";
{
    $mc->set("issue48", "text", 0, 0);
    my $rv =()= eval { $mc->incr('issue48'); };
    ok($@ && $@->delta_badval, "Expected invalid value when incrementing text.");
    $check->('issue48', 0, "text");

    $rv =()= eval { $mc->decr('issue48'); };
    ok($@ && $@->delta_badval, "Expected invalid value when decrementing text.");
    $check->('issue48', 0, "text");
}


# diag "Test decrement";
$mc->flush;
is($mc->incr("x", undef, 5), 5, "Initial value");
is($mc->decr("x"), 4, "Decrease by one");
is($mc->decr("x", 211), 0, "Floor is zero");

{
    # diag "bug21";
    $mc->add("bug21", "9223372036854775807", 0, 0);
    is($mc->incr("bug21"), 9223372036854775808, "First incr for bug21.");
    is($mc->incr("bug21"), 9223372036854775809, "Second incr for bug21.");
    is($mc->decr("bug21"), 9223372036854775808, "Decr for bug21.");
}

{
    # diag "CAS";
    $mc->flush;

    {
        my $rv =()= eval { $mc->set("x", "bad value", 19, 5, 0x7FFFFFF) };
        is($rv, 0, "Empty return on expected failure");
        ok($@->not_found, "Error was 'not found' as expected");
    }

    my ($r, $rcas) = $mc->add("x", "original value", 5, 19);

    my ($flags, $val, $i) = $mc->get("x");
    is($val, "original value", "->gets returned proper value");
    is($rcas, $i, "Add CAS matched.");

    {
        my $rv =()= eval { $mc->set("x", "broken value", 19, 5, $i+1) };
        is($rv, 0, "Empty return on expected failure (1)");
        ok($@->exists, "Expected error state of 'exists' (1)");
    }

    ($r, $rcas) = $mc->set("x", "new value", 19, 5, $i);

    my ($newflags, $newval, $newi) = $mc->get("x");
    is($newval, "new value", "CAS properly overwrote value");
    is($rcas, $newi, "Get CAS matched.");

    {
        my $rv =()= eval { $mc->set("x", "replay value", 19, 5,  $i) };
        is($rv, 0, "Empty return on expected failure (2)");
        ok($@->exists, "Expected error state of 'exists' (2)");
    }
}

# diag "Silent set.";
$mc->silent_mutation(::CMD_SETQ, 'silentset', 'silentsetval');

# diag "Silent add.";
$mc->silent_mutation(::CMD_ADDQ, 'silentadd', 'silentaddval');

# diag "Silent replace.";
{
    my $key = "silentreplace";
    my $extra = pack "NN", 829, 0;
    $empty->($key);
    # $mc->send_silent(::CMD_REPLACEQ, $key, 'somevalue', 7278552, $extra, 0);
    # $empty->($key);

    $mc->add($key, "xval", 831, 0);
    $check->($key, 831, 'xval');

    $mc->send_silent(::CMD_REPLACEQ, $key, 'somevalue', 7278552, $extra, 0);
    $check->($key, 829, 'somevalue');
}

# diag "Silent delete";
{
    my $key = "silentdelete";
    $empty->($key);
    $mc->set($key, "some val", 19, 0);
    $mc->send_silent(::CMD_DELETEQ, $key, '', 772);
    $empty->($key);
}

# diag "Silent increment";
{
    my $key = "silentincr";
    my $opaque = 98428747;
    $empty->($key);
    $mc->silent_incrdecr(::CMD_INCREMENTQ, $key, 0, 0, 0);
    is($mc->incr($key, 0), 0, "First call is 0");

    $mc->silent_incrdecr(::CMD_INCREMENTQ, $key, 8, 0, 0);
    is($mc->incr($key, 0), 8);
}

# diag "Silent decrement";
{
    my $key = "silentdecr";
    my $opaque = 98428147;
    $empty->($key);
    $mc->silent_incrdecr(::CMD_DECREMENTQ, $key, 0, 185, 0);
    is($mc->incr($key, 0), 185);

    $mc->silent_incrdecr(::CMD_DECREMENTQ, $key, 8, 0, 0);
    is($mc->incr($key, 0), 177);
}

# diag "Silent flush";
{
    my %stats1 = $mc->stats('');

    $set->('x', 5, 19, "somevaluex");
    $set->('y', 5, 17, "somevaluey");
    $mc->send_silent(::CMD_FLUSHQ, '', '', 2775256);
    $empty->('x');
    $empty->('y');

    my %stats2 = $mc->stats('');
    is($stats2{'cmd_flush'}, $stats1{'cmd_flush'} + 1,
       "Stats not updated on a binary quiet flush");
}

# diag "Append";
{
    my $key = "appendkey";
    my $value = "some value";
    $set->($key, 8, 19, $value);
    $mc->_append_prepend(::CMD_APPEND, $key, " more");
    $check->($key, 19, $value . " more");
}

# diag "Prepend";
{
    my $key = "prependkey";
    my $value = "some value";
    $set->($key, 8, 19, $value);
    $mc->_append_prepend(::CMD_PREPEND, $key, "prefixed ");
    $check->($key, 19, "prefixed " . $value);
}

# diag "Silent append";
{
    my $key = "appendqkey";
    my $value = "some value";
    $set->($key, 8, 19, $value);
    $mc->send_silent(::CMD_APPENDQ, $key, " more", 7284492);
    $check->($key, 19, $value . " more");
}

# diag "Silent prepend";
{
    my $key = "prependqkey";
    my $value = "some value";
    $set->($key, 8, 19, $value);
    $mc->send_silent(::CMD_PREPENDQ, $key, "prefixed ", 7284492);
    $check->($key, 19, "prefixed " . $value);
}

# diag "Leaky binary get test.";
# # http://code.google.com/p/memcached/issues/detail?id=16
{
    # Get a new socket so we can speak text to it.
    my $sock = $server->new_sock;
    my $max = 1024 * 1024;
    my $big = "a big value that's > .5M and < 1M. ";
    while (length($big) * 2 < $max) {
        $big = $big . $big;
    }
    my $biglen = length($big);

    for(1..100) {
        my $key = "some_key_$_";
        # print STDERR "Key is $key\n";
        # print $sock "set $key 0 0 $vallen\r\n$value\r\n";
        print $sock "set $key 0 0 $biglen\r\n$big\r\n";
        is(scalar <$sock>, "STORED\r\n", "stored big");
        my ($f, $v, $c) = $mc->get($key);
    }
}

# diag "Test stats settings."
{
    my %stats = $mc->stats('settings');

    is(1024, $stats{'maxconns'});
    is('NULL', $stats{'domain_socket'});
    is('on', $stats{'evictions'});
    is('yes', $stats{'cas_enabled'});
}

# diag "Test quit commands.";
{
    my $s2 = new_memcached();
    my $mc2 = MC::Client->new($s2);
    $mc2->send_command(CMD_QUITQ, '', '', 0, '', 0);

    # Five seconds ought to be enough to get hung up on.
    my $oldalarmt = alarm(5);

    # Verify we can't read anything.
    my $bytesread = -1;
    eval {
        local $SIG{'ALRM'} = sub { die "timeout" };
        my $data = "";
        $bytesread = sysread($mc2->{socket}, $data, 24),
    };
    is($bytesread, 0, "Read after quit.");

    # Restore signal stuff.
    alarm($oldalarmt);
}

# diag "Test protocol boundary overruns";
{
    use List::Util qw[min];
    # Attempting some protocol overruns by toying around with the edge
    # of the data buffer at a few different sizes.  This assumes the
    # boundary is at or around 2048 bytes.
    for (my $i = 1900; $i < 2100; $i++) {
        my $k = "test_key_$i";
        my $v = 'x' x $i;
        # diag "Trying $i $k";
        my $extra = pack "NN", 82, 0;
        my $data = $mc->build_command(::CMD_SETQ, $k, $v, 0, $extra, 0);
        $data .= $mc->build_command(::CMD_SETQ, "alt_$k", "blah", 0, $extra, 0);
        if (length($data) > 2024) {
            for (my $j = 2024; $j < min(2096, length($data)); $j++) {
                $mc->{socket}->send(substr($data, 0, $j));
                $mc->flush_socket;
                sleep(0.001);
                $mc->{socket}->send(substr($data, $j));
                $mc->flush_socket;
            }
        } else {
            $mc->{socket}->send($data);
        }
        $mc->flush_socket;
        $check->($k, 82, $v);
        $check->("alt_$k", 82, "blah");
    }
}

# Along with the assertion added to the code to verify we're staying
# within bounds when we do a stats detail dump (detail turned on at
# the top).
my %stats = $mc->stats('detail dump');

# This test causes a disconnection.
{
    # diag "Key too large.";
    my $key = "x" x 365;
    eval {
        $mc->get($key, 'should die', 10, 10);
    };
    ok($@->einval, "Invalid key length");
}

# ######################################################################
# Test ends around here.
# ######################################################################

package MC::Client;

use strict;
use warnings;
use fields qw(socket);
use IO::Socket::INET;

sub new {
    my $self = shift;
    my ($s) = @_;
    $s = $server unless defined $s;
    my $sock = $s->sock;
    $self = fields::new($self);
    $self->{socket} = $sock;
    return $self;
}

sub build_command {
    my $self = shift;
    die "Not enough args to send_command" unless @_ >= 4;
    my ($cmd, $key, $val, $opaque, $extra_header, $cas) = @_;

    $extra_header = '' unless defined $extra_header;
    my $keylen    = length($key);
    my $vallen    = length($val);
    my $extralen  = length($extra_header);
    my $datatype  = 0;  # field for future use
    my $reserved  = 0;  # field for future use
    my $totallen  = $keylen + $vallen + $extralen;
    my $ident_hi  = 0;
    my $ident_lo  = 0;

    if ($cas) {
        $ident_hi = int($cas / 2 ** 32);
        $ident_lo = int($cas % 2 ** 32);
    }

    my $msg = pack(::REQ_PKT_FMT, ::REQ_MAGIC, $cmd, $keylen, $extralen,
                   $datatype, $reserved, $totallen, $opaque, $ident_hi,
                   $ident_lo);
    my $full_msg = $msg . $extra_header . $key . $val;
    return $full_msg;
}

sub send_command {
    my $self = shift;
    die "Not enough args to send_command" unless @_ >= 4;
    my ($cmd, $key, $val, $opaque, $extra_header, $cas) = @_;

    my $full_msg = $self->build_command($cmd, $key, $val, $opaque, $extra_header, $cas);

    my $sent = $self->{socket}->send($full_msg);
    die("Send failed:  $!") unless $sent;
    if($sent != length($full_msg)) {
        die("only sent $sent of " . length($full_msg) . " bytes");
    }
}

sub flush_socket {
    my $self = shift;
    $self->{socket}->flush;
}

# Send a silent command and ensure it doesn't respond.
sub send_silent {
    my $self = shift;
    die "Not enough args to send_silent" unless @_ >= 4;
    my ($cmd, $key, $val, $opaque, $extra_header, $cas) = @_;

    $self->send_command($cmd, $key, $val, $opaque, $extra_header, $cas);
    $self->send_command(::CMD_NOOP, '', '', $opaque + 1);

    my ($ropaque, $data) = $self->_handle_single_response;
    Test::More::is($ropaque, $opaque + 1);
}

sub silent_mutation {
    my $self = shift;
    my ($cmd, $key, $value) = @_;

    $empty->($key);
    my $extra = pack "NN", 82, 0;
    $mc->send_silent($cmd, $key, $value, 7278552, $extra, 0);
    $check->($key, 82, $value);
}

sub _handle_single_response {
    my $self = shift;
    my $myopaque = shift;

    $self->{socket}->recv(my $response, ::MIN_RECV_BYTES);
    Test::More::is(length($response), ::MIN_RECV_BYTES, "Expected read length");

    my ($magic, $cmd, $keylen, $extralen, $datatype, $status, $remaining,
        $opaque, $ident_hi, $ident_lo) = unpack(::RES_PKT_FMT, $response);
    Test::More::is($magic, ::RES_MAGIC, "Got proper response magic");

    my $cas = ($ident_hi * 2 ** 32) + $ident_lo;

    return ($opaque, '', $cas, 0) if($remaining == 0);

    # fetch the value
    my $rv="";
    while($remaining - length($rv) > 0) {
        $self->{socket}->recv(my $buf, $remaining - length($rv));
        $rv .= $buf;
    }
    if(length($rv) != $remaining) {
        my $found = length($rv);
        die("Expected $remaining bytes, got $found");
    }

    if (defined $myopaque) {
        Test::More::is($opaque, $myopaque, "Expected opaque");
    } else {
        Test::More::pass("Implicit pass since myopaque is undefined");
    }

    if ($status) {
        die MC::Error->new($status, $rv);
    }

    return ($opaque, $rv, $cas, $keylen);
}

sub _do_command {
    my $self = shift;
    die unless @_ >= 3;
    my ($cmd, $key, $val, $extra_header, $cas) = @_;

    $extra_header = '' unless defined $extra_header;
    my $opaque = int(rand(2**32));
    $self->send_command($cmd, $key, $val, $opaque, $extra_header, $cas);
    my (undef, $rv, $rcas) = $self->_handle_single_response($opaque);
    return ($rv, $rcas);
}

sub _incrdecr_header {
    my $self = shift;
    my ($amt, $init, $exp) = @_;

    my $amt_hi = int($amt / 2 ** 32);
    my $amt_lo = int($amt % 2 ** 32);

    my $init_hi = int($init / 2 ** 32);
    my $init_lo = int($init % 2 ** 32);

    my $extra_header = pack(::INCRDECR_PKT_FMT, $amt_hi, $amt_lo, $init_hi,
                            $init_lo, $exp);

    return $extra_header;
}

sub _incrdecr {
    my $self = shift;
    my ($cmd, $key, $amt, $init, $exp) = @_;

    my ($data, undef) = $self->_do_command($cmd, $key, '',
                                           $self->_incrdecr_header($amt, $init, $exp));

    my $header = substr $data, 0, 8, '';
    my ($resp_hi, $resp_lo) = unpack "NN", $header;
    my $resp = ($resp_hi * 2 ** 32) + $resp_lo;

    return $resp;
}

sub silent_incrdecr {
    my $self = shift;
    my ($cmd, $key, $amt, $init, $exp) = @_;
    my $opaque = 8275753;

    $mc->send_silent($cmd, $key, '', $opaque,
                     $mc->_incrdecr_header($amt, $init, $exp));
}

sub stats {
    my $self = shift;
    my $key  = shift;
    my $cas = 0;
    my $opaque = int(rand(2**32));
    $self->send_command(::CMD_STAT, $key, '', $opaque, '', $cas);

    my %rv = ();
    my $found_key = '';
    my $found_val = '';
    do {
        my ($op, $data, $cas, $keylen) = $self->_handle_single_response($opaque);
        if($keylen > 0) {
            $found_key = substr($data, 0, $keylen);
            $found_val = substr($data, $keylen);
            $rv{$found_key} = $found_val;
        } else {
            $found_key = '';
        }
    } while($found_key ne '');
    return %rv;
}

sub get {
    my $self = shift;
    my $key  = shift;
    my ($rv, $cas) = $self->_do_command(::CMD_GET, $key, '', '');

    my $header = substr $rv, 0, 4, '';
    my $flags  = unpack("N", $header);

    return ($flags, $rv, $cas);
}

sub get_multi {
    my $self = shift;
    my @keys = @_;

    for (my $i = 0; $i < @keys; $i++) {
        $self->send_command(::CMD_GETQ, $keys[$i], '', $i, '', 0);
    }

    my $terminal = @keys + 10;
    $self->send_command(::CMD_NOOP, '', '', $terminal);

    my %return;
    while (1) {
        my ($opaque, $data) = $self->_handle_single_response;
        last if $opaque == $terminal;

        my $header = substr $data, 0, 4, '';
        my $flags  = unpack("N", $header);

        $return{$keys[$opaque]} = [$flags, $data];
    }

    return %return if wantarray;
    return \%return;
}

sub version {
    my $self = shift;
    return $self->_do_command(::CMD_VERSION, '', '');
}

sub flush {
    my $self = shift;
    return $self->_do_command(::CMD_FLUSH, '', '');
}

sub add {
    my $self = shift;
    my ($key, $val, $flags, $expire) = @_;
    my $extra_header = pack "NN", $flags, $expire;
    my $cas = 0;
    return $self->_do_command(::CMD_ADD, $key, $val, $extra_header, $cas);
}

sub set {
    my $self = shift;
    my ($key, $val, $flags, $expire, $cas) = @_;
    my $extra_header = pack "NN", $flags, $expire;
    return $self->_do_command(::CMD_SET, $key, $val, $extra_header, $cas);
}

sub _append_prepend {
    my $self = shift;
    my ($cmd, $key, $val, $cas) = @_;
    return $self->_do_command($cmd, $key, $val, '', $cas);
}

sub replace {
    my $self = shift;
    my ($key, $val, $flags, $expire) = @_;
    my $extra_header = pack "NN", $flags, $expire;
    my $cas = 0;
    return $self->_do_command(::CMD_REPLACE, $key, $val, $extra_header, $cas);
}

sub delete {
    my $self = shift;
    my ($key) = @_;
    return $self->_do_command(::CMD_DELETE, $key, '');
}

sub incr {
    my $self = shift;
    my ($key, $amt, $init, $exp) = @_;
    $amt = 1 unless defined $amt;
    $init = 0 unless defined $init;
    $exp = 0 unless defined $exp;

    return $self->_incrdecr(::CMD_INCR, $key, $amt, $init, $exp);
}

sub decr {
    my $self = shift;
    my ($key, $amt, $init, $exp) = @_;
    $amt = 1 unless defined $amt;
    $init = 0 unless defined $init;
    $exp = 0 unless defined $exp;

    return $self->_incrdecr(::CMD_DECR, $key, $amt, $init, $exp);
}

sub noop {
    my $self = shift;
    return $self->_do_command(::CMD_NOOP, '', '');
}

package MC::Error;

use strict;
use warnings;

use constant ERR_UNKNOWN_CMD  => 0x81;
use constant ERR_NOT_FOUND    => 0x1;
use constant ERR_EXISTS       => 0x2;
use constant ERR_TOO_BIG      => 0x3;
use constant ERR_EINVAL       => 0x4;
use constant ERR_NOT_STORED   => 0x5;
use constant ERR_DELTA_BADVAL => 0x6;

use overload '""' => sub {
    my $self = shift;
    return "Memcache Error ($self->[0]): $self->[1]";
};

sub new {
    my $class = shift;
    my $error = [@_];
    my $self = bless $error, (ref $class || $class);

    return $self;
}

sub not_found {
    my $self = shift;
    return $self->[0] == ERR_NOT_FOUND;
}

sub exists {
    my $self = shift;
    return $self->[0] == ERR_EXISTS;
}

sub too_big {
    my $self = shift;
    return $self->[0] == ERR_TOO_BIG;
}

sub delta_badval {
    my $self = shift;
    return $self->[0] == ERR_DELTA_BADVAL;
}

sub einval {
    my $self = shift;
    return $self->[0] == ERR_EINVAL;
}

# vim: filetype=perl

