#!/usr/bin/perl

use strict;
use warnings;
use Cwd;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

my $supports_sasl = supports_sasl();

use Test::More;

if (supports_sasl()) {
    plan tests => 25;
} else {
    plan tests => 1;
    eval {
        my $server = new_memcached("-S");
    };
    ok($@, "Died with illegal -S args when SASL is not supported.");
    exit 0;
}

eval {
    my $server = new_memcached("-S -B auto");
};
ok($@, "SASL shouldn't be used with protocol auto negotiate");

eval {
    my $server = new_memcached("-S -B ascii");
};
ok($@, "SASL isn't implemented in the ascii protocol");

eval {
    my $server = new_memcached("-S -B binary -B ascii");
};
ok($@, "SASL isn't implemented in the ascii protocol");

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

use constant CMD_SASL_LIST_MECHS    => 0x20;
use constant CMD_SASL_AUTH          => 0x21;
use constant CMD_SASL_STEP          => 0x22;
use constant ERR_AUTH_ERROR   => 0x20;


# REQ and RES formats are divided even though they currently share
# the same format, since they _could_ differ in the future.
use constant REQ_PKT_FMT      => "CCnCCnNNNN";
use constant RES_PKT_FMT      => "CCnCCnNNNN";
use constant INCRDECR_PKT_FMT => "NNNNN";
use constant MIN_RECV_BYTES   => length(pack(RES_PKT_FMT));
use constant REQ_MAGIC        => 0x80;
use constant RES_MAGIC        => 0x81;

my $pwd=getcwd;
$ENV{'SASL_CONF_PATH'} = "$pwd/t/sasl";

my $server = new_memcached('-B binary -S ');

my $mc = MC::Client->new;

my $check = sub {
    my ($key, $orig_val) = @_;
    my ($status, $val, $cas) = $mc->get($key);

    if ($val =~ /^\d+$/) {
        cmp_ok($val,'==', $orig_val, "$val = $orig_val");
    }
    else {
        cmp_ok($val, 'eq', $orig_val, "$val = $orig_val");
    }
};

my $set = sub {
    my ($key, $orig_value, $exp) = @_;
    $exp = defined $exp ? $exp : 0;
    my ($status, $rv)= $mc->set($key, $orig_value, $exp);
    $check->($key, $orig_value);
};

my $empty = sub {
    my $key = shift;
    my ($status,$rv) =()= eval { $mc->get($key) };
    #if ($status == ERR_AUTH_ERROR) {
    #    ok($@->auth_error, "Not authorized to connect");
    #}
    #else {
    #    ok($@->not_found, "We got a not found error when we expected one");
    #}
    if ($status) {
        ok($@->not_found, "We got a not found error when we expected one");
    }
};

my $delete = sub {
    my ($key, $when) = @_;
    $mc->delete($key, $when);
    $empty->($key);
};

# BEGIN THE TEST
ok($server, "started the server");

my $v = $mc->version;
ok(defined $v && length($v), "Proper version: $v");

# list mechs
my $mechs= $mc->list_mechs();
Test::More::cmp_ok($mechs, 'eq', 'CRAM-MD5 PLAIN', "list_mechs $mechs");

# this should fail, not authenticated
{
    my ($status, $val)= $mc->set('x', "somevalue");
    ok($status, "this fails to authenticate");
    cmp_ok($status,'==',ERR_AUTH_ERROR, "error code matches");
}
$empty->('x');
{
    my $mc = MC::Client->new;
    my ($status, $val) = $mc->delete('x');
    ok($status, "this fails to authenticate");
    cmp_ok($status,'==',ERR_AUTH_ERROR, "error code matches");
}
$empty->('x');
{
    my $mc = MC::Client->new;
    my ($status, $val)= $mc->set('x', "somevalue");
    ok($status, "this fails to authenticate");
    cmp_ok($status,'==',ERR_AUTH_ERROR, "error code matches");
}
$empty->('x');
{
    my $mc = MC::Client->new;
    my ($status, $val)=  $mc->flush('x');
    ok($status, "this fails to authenticate");
    cmp_ok($status,'==',ERR_AUTH_ERROR, "error code matches");
}
$empty->('x');

# Build the auth DB for testing.
my $sasldb = '/tmp/test-memcached.sasldb';
unlink $sasldb;
system("echo testpass | saslpasswd2 -a memcached -c -p testuser");

$mc = MC::Client->new;

# Attempt a bad auth mech.
is ($mc->authenticate('testuser', 'testpass', "X" x 40), 0x4, "bad mech");

# Attempt bad authentication.
is ($mc->authenticate('testuser', 'wrongpassword'), 0x20, "bad auth");

# Now try good authentication and make the tests work.
is ($mc->authenticate('testuser', 'testpass'), 0, "authenticated");
# these should work
{
    my ($status, $val)= $mc->set('x', "somevalue");
    ok(! $status);
}
$check->('x','somevalue');

{
    my ($status, $val)= $mc->delete('x');
    ok(! $status);
}
$empty->('x');

{
    my ($status, $val)= $mc->set('x', "somevalue");
    ok(! $status);
}
$check->('x','somevalue');

{
    my ($status, $val)=  $mc->flush('x');
    ok(! $status);
}
$empty->('x');

# check the SASL stats, make sure they track things correctly
# note: the enabled or not is presence checked in stats.t

# while authenticated, get current counter
#
# My initial approach was going to be to get current counts, reauthenticate
# and fail, followed by a reauth successfully so I'd know what happened.
# Reauthentication is currently unsupported, so it doesn't work that way at the
# moment.  Adding tests may break this.

{
    my %stats = $mc->stats('');
    is ($stats{'auth_cmds'}, 2, "auth commands counted");
    is ($stats{'auth_errors'}, 1, "auth errors correct");
}


# Along with the assertion added to the code to verify we're staying
# within bounds when we do a stats detail dump (detail turned on at
# the top).
# my %stats = $mc->stats('detail dump');

# ######################################################################
# Test ends around here.
# ######################################################################

package MC::Client;

use strict;
use warnings;
use fields qw(socket);
use IO::Socket::INET;

use constant ERR_AUTH_ERROR   => 0x20;

sub new {
    my $self = shift;
    my ($s) = @_;
    $s = $server unless defined $s;
    my $sock = $s->sock;
    $self = fields::new($self);
    $self->{socket} = $sock;
    return $self;
}

sub authenticate {
    my ($self, $user, $pass, $mech)= @_;
    $mech ||= 'PLAIN';
    my $buf = sprintf("%c%s%c%s", 0, $user, 0, $pass);
    my ($status, $rv, undef) = $self->_do_command(::CMD_SASL_AUTH, $mech, $buf, '');
    return $status;
}
sub list_mechs {
    my ($self)= @_;
    my ($status, $rv, undef) = $self->_do_command(::CMD_SASL_LIST_MECHS, '', '', '');
    return join(" ", sort(split(/\s+/, $rv)));
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

    my ($ropaque, $status, $data) = $self->_handle_single_response;
    Test::More::is($ropaque, $opaque + 1);
}

sub silent_mutation {
    my $self = shift;
    my ($cmd, $key, $value) = @_;

    $empty->($key);
    my $extra = pack "NN", 82, 0;
    $mc->send_silent($cmd, $key, $value, 7278552, $extra, 0);
    $check->($key, $value);
}

sub _handle_single_response {
    my $self = shift;
    my $myopaque = shift;

    $self->{socket}->recv(my $response, ::MIN_RECV_BYTES);

    my ($magic, $cmd, $keylen, $extralen, $datatype, $status, $remaining,
        $opaque, $ident_hi, $ident_lo) = unpack(::RES_PKT_FMT, $response);

    return ($opaque, '', '', '', 0) if not defined $remaining;
    return ($opaque, '', '', '', 0) if ($remaining == 0);

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

    my $cas = ($ident_hi * 2 ** 32) + $ident_lo;

    #if ($status) {
        #die MC::Error->new($status, $rv);
    #}

    return ($opaque, $status, $rv, $cas, $keylen);
}

sub _do_command {
    my $self = shift;
    die unless @_ >= 3;
    my ($cmd, $key, $val, $extra_header, $cas) = @_;

    $extra_header = '' unless defined $extra_header;
    my $opaque = int(rand(2**32));
    $self->send_command($cmd, $key, $val, $opaque, $extra_header, $cas);
    my (undef, $status, $rv, $rcas) = $self->_handle_single_response($opaque);
    return ($status, $rv, $rcas);
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

    my ($status, $data, undef) = $self->_do_command($cmd, $key, '',
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
    my $status= 0;
    do {
        my ($op, $status, $data, $cas, $keylen) = $self->_handle_single_response($opaque);
        if ($keylen > 0) {
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
    my ($status, $rv, $cas) = $self->_do_command(::CMD_GET, $key, '', '');

    my $header = substr $rv, 0, 4, '';
    my $flags  = unpack("N", $header);

    return ($status, $rv);
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
    my $status = 0;
    while (1) {
        my ($opaque, $status, $data) = $self->_handle_single_response;
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
    my $flags = 0;
    my $cas = 0;
    my ($key, $val, $expire) = @_;
    $expire = defined $expire ? $expire : 0;
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
use constant ERR_AUTH_ERROR   => 0x20;

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

sub auth_error {
    my $self = shift;
    return $self->[0] == ERR_AUTH_ERROR;
}

unlink $sasldb;

# vim: filetype=perl

