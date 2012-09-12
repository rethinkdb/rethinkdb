#!/usr/bin/perl

use strict;
use Test::More tests => 48;
use FindBin qw($Bin);
use lib "$Bin/lib";
use MemcachedTest;

use constant IS_ASCII         => 0;
use constant IS_BINARY        => 1;
use constant ENTRY_EXISTS     => 0;
use constant ENTRY_MISSING    => 1;
use constant BIN_REQ_MAGIC    => 0x80;
use constant BIN_RES_MAGIC    => 0x81;
use constant CMD_GET          => 0x00;
use constant CMD_SET          => 0x01;
use constant CMD_ADD          => 0x02;
use constant CMD_REPLACE      => 0x03;
use constant CMD_DELETE       => 0x04;
use constant CMD_INCR         => 0x05;
use constant CMD_DECR         => 0x06;
use constant CMD_APPEND       => 0x0E;
use constant CMD_PREPEND      => 0x0F;
use constant REQ_PKT_FMT      => "CCnCCnNNNN";
use constant RES_PKT_FMT      => "CCnCCnNNNN";
use constant INCRDECR_PKT_FMT => "NNNNN";
use constant MIN_RECV_BYTES   => length(pack(RES_PKT_FMT));


my $server = new_memcached();
my $sock = $server->sock;

# set foo (and should get it)
print $sock "set foo 0 0 6\r\nfooval\r\n";
is(scalar <$sock>, "STORED\r\n", "stored foo");
mem_get_is($sock, "foo", "fooval");

my $usock = $server->new_udp_sock
    or die "Can't bind : $@\n";

# testing sequence of request ids
for my $offt (1, 1, 2) {
    my $req = 160 + $offt;
    my $res = send_udp_request($usock, $req, "get foo\r\n");
    ok($res, "got result");
    is(keys %$res, 1, "one key (one packet)");
    ok($res->{0}, "only got seq number 0");
    is(substr($res->{0}, 8), "VALUE foo 0 6\r\nfooval\r\nEND\r\n");
    is(hexify(substr($res->{0}, 0, 2)), hexify(pack("n", $req)), "udp request number in response ($req) is correct");
}

# op tests
for my $prot (::IS_ASCII,::IS_BINARY) {
    udp_set_test($prot,45,"aval$prot","1",0,0);
    udp_set_test($prot,45,"bval$prot","abcd" x 1024,0,0);
    udp_get_test($prot,45,"aval$prot","1",::ENTRY_EXISTS);
    udp_get_test($prot,45,"404$prot","1",::ENTRY_MISSING);
    udp_incr_decr_test($prot,45,"aval$prot","1","incr",1);
    udp_incr_decr_test($prot,45,"aval$prot","1","decr",2);
    udp_delete_test($prot,45,"aval$prot");
}

sub udp_set_test {
    my ($protocol, $req_id, $key, $value, $flags, $exp) = @_;
    my $req = "";
    my $val_len = length($value);

    if ($protocol == ::IS_ASCII) {
        $req = "set $key $flags $exp $val_len\r\n$value\r\n";
    } elsif ($protocol == ::IS_BINARY) {
        my $key_len = length($key);
        my $extra = pack "NN",$flags,$exp;
        my $extra_len = length($extra);
        my $total_len = $val_len + $extra_len + $key_len;
        $req = pack(::REQ_PKT_FMT, ::BIN_REQ_MAGIC, ::CMD_SET, $key_len, $extra_len, 0, 0, $total_len, 0, 0, 0);
        $req .=  $extra . $key . $value;
    }

    my $datagrams = send_udp_request($usock, $req_id, $req);
    my $resp = construct_udp_message($datagrams);

    if ($protocol == ::IS_ASCII) {
        is($resp,"STORED\r\n","Store key $key using ASCII protocol");
    } elsif ($protocol == ::IS_BINARY) {
        my ($resp_magic, $resp_op_code, $resp_key_len, $resp_extra_len, $resp_data_type, $resp_status, $resp_total_len,
            $resp_opaque, $resp_ident_hi, $resp_ident_lo) = unpack(::RES_PKT_FMT, $resp);
        is($resp_status,"0","Store key $key using binary protocol");
    }
}

sub udp_get_test {
    my ($protocol, $req_id, $key, $value, $exists) = @_;
    my $key_len = length($key);
    my $value_len = length($value);
    my $req = "";

    if ($protocol == ::IS_ASCII) {
        $req = "get $key\r\n";
    } elsif ($protocol == ::IS_BINARY) {
        $req = pack(::REQ_PKT_FMT, ::BIN_REQ_MAGIC, ::CMD_GET, $key_len, 0, 0, 0, $key_len, 0, 0, 0);
        $req .= $key;
    }

    my $datagrams = send_udp_request($usock, $req_id, $req);
    my $resp = construct_udp_message($datagrams);

    if ($protocol == ::IS_ASCII) {
        if ($exists == ::ENTRY_EXISTS) {
            is($resp,"VALUE $key 0 $value_len\r\n$value\r\nEND\r\n","Retrieve entry with key $key using ASCII protocol");
        } else {
            is($resp,"END\r\n","Retrieve non existing entry with key $key using ASCII protocol");
        }
    } elsif ($protocol == ::IS_BINARY) {
        my ($resp_magic, $resp_op_code, $resp_key_len, $resp_extra_len, $resp_data_type, $resp_status, $resp_total_len,
            $resp_opaque, $resp_ident_hi, $resp_ident_lo) = unpack(::RES_PKT_FMT, $resp);
        if ($exists == ::ENTRY_EXISTS) {
            is($resp_status,"0","Retrieve entry with key $key using binary protocol");
            is(substr($resp,::MIN_RECV_BYTES + $resp_extra_len + $resp_key_len, $value_len),$value,"Value for key $key retrieved with binary protocol matches");
        } else {
            is($resp_status,"1","Retrieve non existing entry with key $key using binary protocol");
        }
    }
}

sub udp_delete_test {
    my ($protocol, $req_id, $key) = @_;
    my $req = "";
    my $key_len = length($key);

    if ($protocol == ::IS_ASCII) {
        $req = "delete $key\r\n";
    } elsif ($protocol == ::IS_BINARY) {
        $req = pack(::REQ_PKT_FMT, ::BIN_REQ_MAGIC, ::CMD_DELETE, $key_len, 0, 0, 0, $key_len, 0, 0, 0);
        $req .= $key;
    }

    my $datagrams = send_udp_request($usock, $req_id, $req);
    my $resp = construct_udp_message($datagrams);

    if ($protocol == ::IS_ASCII) {
        is($resp,"DELETED\r\n","Delete key $key using ASCII protocol");
    } elsif ($protocol == ::IS_BINARY) {
        my ($resp_magic, $resp_op_code, $resp_key_len, $resp_extra_len, $resp_data_type, $resp_status, $resp_total_len,
            $resp_opaque, $resp_ident_hi, $resp_ident_lo) = unpack(::RES_PKT_FMT, $resp);
        is($resp_status,"0","Delete key $key using binary protocol");
    }
}

sub udp_incr_decr_test {
    my ($protocol, $req_id, $key, $val, $optype, $init_val) = @_;
    my $req = "";
    my $key_len = length($key);
    my $expected_value = 0;
    my $acmd = "incr";
    my $bcmd = ::CMD_INCR;
    if ($optype eq "incr") {
        $expected_value = $init_val + $val;
    } else {
        $acmd = "decr";
        $bcmd = ::CMD_DECR;
        $expected_value = $init_val - $val;
    }

    if ($protocol == ::IS_ASCII) {
        $req = "$acmd $key $val\r\n";
    } elsif ($protocol == ::IS_BINARY) {
        my $extra = pack(::INCRDECR_PKT_FMT, ($val / 2 ** 32),($val % 2 ** 32), 0, 0, 0);
        my $extra_len = length($extra);
        $req = pack(::REQ_PKT_FMT, ::BIN_REQ_MAGIC, $bcmd, $key_len, $extra_len, 0, 0, $key_len + $extra_len, 0, 0, 0);
        $req .= $extra . $key;
    }

    my $datagrams = send_udp_request($usock, $req_id, $req);
    my $resp = construct_udp_message($datagrams);

    if ($protocol == ::IS_ASCII) {
        is($resp,"$expected_value\r\n","perform $acmd math operation on key $key with ASCII protocol");
    } elsif ($protocol == ::IS_BINARY) {
        my ($resp_magic, $resp_op_code, $resp_key_len, $resp_extra_len, $resp_data_type, $resp_status, $resp_total_len,
            $resp_opaque, $resp_ident_hi, $resp_ident_lo) = unpack(::RES_PKT_FMT, $resp);
        is($resp_status,"0","perform $acmd math operation on key $key with binary protocol");
        my ($resp_hi,$resp_lo) = unpack("NN",substr($resp,::MIN_RECV_BYTES + $resp_extra_len + $resp_key_len,
                                                    $resp_total_len - $resp_extra_len - $resp_key_len));
        is(($resp_hi * 2 ** 32) + $resp_lo,$expected_value,"validate result of binary protocol math operation $acmd . Expected value $expected_value")
    }
}

sub construct_udp_message {
    my $datagrams = shift;
    my $num_datagram = keys (%$datagrams);
    my $msg = "";
    my $cur_dg ="";
    my $cur_udp_header ="";
    for (my $cur_dg_index = 0; $cur_dg_index < $num_datagram; $cur_dg_index++) {
        $cur_dg = %$datagrams->{$cur_dg_index};
        isnt($cur_dg,"","missing datagram for segment $cur_dg_index");
        $cur_udp_header=substr($cur_dg, 0, 8);
        $msg .= substr($cur_dg,8);
    }
    return $msg;
}

sub hexify {
    my $val = shift;
    $val =~ s/(.)/sprintf("%02x", ord($1))/egs;
    return $val;
}

# returns undef on select timeout, or hashref of "seqnum" -> payload (including headers)
# verifies that resp_id is equal to id sent in request
# ensures consistency in num packets that make up response
sub send_udp_request {
    my ($sock, $reqid, $req) = @_;

    my $pkt = pack("nnnn", $reqid, 0, 1, 0);  # request id (opaque), seq num, #packets, reserved (must be 0)
    $pkt .= $req;
    my $fail = sub {
        my $msg = shift;
        warn "  FAILING send_udp because: $msg\n";
        return undef;
    };
    return $fail->("send") unless send($sock, $pkt, 0);

    my $ret = {};

    my $got = 0;   # packets got
    my $numpkts = undef;

    while (!defined($numpkts) || $got < $numpkts) {
        my $rin = '';
        vec($rin, fileno($sock), 1) = 1;
        my $rout;
        return $fail->("timeout after $got packets") unless
            select($rout = $rin, undef, undef, 1.5);

        my $res;
        my $sender = $sock->recv($res, 1500, 0);
        my ($resid, $seq, $this_numpkts, $resv) = unpack("nnnn", substr($res, 0, 8));
        die "Response ID of $resid doesn't match request if of $reqid" unless $resid == $reqid;
        die "Reserved area not zero" unless $resv == 0;
        die "num packets changed midstream!" if defined $numpkts && $this_numpkts != $numpkts;
        $numpkts = $this_numpkts;
        $ret->{$seq} = $res;
        $got++;
    }
    return $ret;
}


__END__
    $sender = recv($usock, $ans, 1050, 0);

__END__
    $usock->send


    ($hispaddr = recv(SOCKET, $rtime, 4, 0))        || die "recv: $!";
($port, $hisiaddr) = sockaddr_in($hispaddr);
$host = gethostbyaddr($hisiaddr, AF_INET);
$histime = unpack("N", $rtime) - $SECS_of_70_YEARS ;
