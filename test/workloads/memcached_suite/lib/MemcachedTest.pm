package MemcachedTest;
use strict;
use IO::Socket::INET;
use IO::Socket::UNIX;
use Exporter 'import';
use Carp qw(croak);
use vars qw(@EXPORT);

# Instead of doing the substitution with Autoconf, we assume that
# cwd == builddir.
use Cwd;
my $builddir = getcwd;


@EXPORT = qw(new_memcached sleep mem_get_is mem_gets mem_gets_is mem_stats
             supports_sasl free_port);

sub sleep {
    my $n = shift;
    select undef, undef, undef, $n;
}

sub mem_stats {
    my ($sock, $type) = @_;
    $type = $type ? " $type" : "";
    print $sock "stats$type\r\n";
    my $stats = {};
    while (<$sock>) {
        last if /^(\.|END)/;
        /^(STAT|ITEM) (\S+)\s+([^\r\n]+)/;
        #print " slabs: $_";
        $stats->{$2} = $3;
    }
    return $stats;
}

sub mem_get_is {
    # works on single-line values only.  no newlines in value.
    my ($sock_opts, $key, $val, $msg) = @_;
    my $opts = ref $sock_opts eq "HASH" ? $sock_opts : {};
    my $sock = ref $sock_opts eq "HASH" ? $opts->{sock} : $sock_opts;

    my $expect_flags = $opts->{flags} || 0;
    my $dval = defined $val ? "'$val'" : "<undef>";
    $msg ||= "$key == $dval";

    print $sock "get $key\r\n";
    if (! defined $val) {
        my $line = scalar <$sock>;
        if ($line =~ /^VALUE/) {
            $line .= scalar(<$sock>) . scalar(<$sock>);
        }
        Test::More::is($line, "END\r\n", $msg);
    } else {
        my $len = length($val);
        my $body = scalar(<$sock>);
        my $expected = "VALUE $key $expect_flags $len\r\n$val\r\nEND\r\n";
        if (!$body || $body =~ /^END/) {
            Test::More::is($body, $expected, $msg);
            return;
        }
        $body .= scalar(<$sock>) . scalar(<$sock>);
        Test::More::is($body, $expected, $msg);
    }
}

sub mem_gets {
    # works on single-line values only.  no newlines in value.
    my ($sock_opts, $key) = @_;
    my $opts = ref $sock_opts eq "HASH" ? $sock_opts : {};
    my $sock = ref $sock_opts eq "HASH" ? $opts->{sock} : $sock_opts;
    my $val;
    my $expect_flags = $opts->{flags} || 0;

    print $sock "gets $key\r\n";
    my $response = <$sock>;
    if ($response =~ /^END/) {
        return "NOT_FOUND";
    }
    else
    {
        $response =~ /VALUE (.*) (\d+) (\d+) (\d+)/;
        my $flags = $2;
        my $len = $3;
        my $identifier = $4;
        read $sock, $val , $len;
        # get the END
        $_ = <$sock>;
        $_ = <$sock>;

        return ($identifier,$val);
    }

}
sub mem_gets_is {
    # works on single-line values only.  no newlines in value.
    my ($sock_opts, $identifier, $key, $val, $msg) = @_;
    my $opts = ref $sock_opts eq "HASH" ? $sock_opts : {};
    my $sock = ref $sock_opts eq "HASH" ? $opts->{sock} : $sock_opts;

    my $expect_flags = $opts->{flags} || 0;
    my $dval = defined $val ? "'$val'" : "<undef>";
    $msg ||= "$key == $dval";

    print $sock "gets $key\r\n";
    if (! defined $val) {
        my $line = scalar <$sock>;
        if ($line =~ /^VALUE/) {
            $line .= scalar(<$sock>) . scalar(<$sock>);
        }
        Test::More::is($line, "END\r\n", $msg);
    } else {
        my $len = length($val);
        my $body = scalar(<$sock>);
        my $expected = "VALUE $key $expect_flags $len $identifier\r\n$val\r\nEND\r\n";
        if (!$body || $body =~ /^END/) {
            Test::More::is($body, $expected, $msg);
            return;
        }
        $body .= scalar(<$sock>) . scalar(<$sock>);
        Test::More::is($body, $expected, $msg);
    }
}

sub free_port {
    my $type = shift || "tcp";
    my $sock;
    my $port;
    while (!$sock) {
        $port = int(rand(20000)) + 30000;
        $sock = IO::Socket::INET->new(LocalAddr => '127.0.0.1',
                                      LocalPort => $port,
                                      Proto     => $type,
                                      ReuseAddr => 1);
    }
    return $port;
}

sub supports_udp {
    return 0;
}

sub supports_sasl {
    return 0;
}

sub new_memcached {
    #croak("No arguments supported") if @_;
    my $conn = IO::Socket::INET->new(PeerAddr => "127.0.0.1:$ENV{'RUN_PORT'}");
    return Memcached::Handle->new(pid => "NOPID", conn => $conn, udpport => "NOUDP", host => '127.0.0.1', port => $ENV{'RUN_PORT'});
}

############################################################################
package Memcached::Handle;
sub new {
    my ($class, %params) = @_;
    return bless \%params, $class;
}

sub DESTROY {
    my $self = shift;
    #kill 2, $self->{pid};
}

sub stop {
    my $self = shift;
    #kill 15, $self->{pid};
}

sub host { $_[0]{host} }
sub port { $_[0]{port} }
sub udpport { $_[0]{udpport} }

sub sock {
    my $self = shift;

    if ($self->{conn} && ($self->{domainsocket} || getpeername($self->{conn}))) {
        return $self->{conn};
    }
    return $self->new_sock;
}

sub new_sock {
    my $self = shift;
    if ($self->{domainsocket}) {
        return IO::Socket::UNIX->new(Peer => $self->{domainsocket});
    } else {
        return IO::Socket::INET->new(PeerAddr => "$self->{host}:$self->{port}");
    }
}

sub new_udp_sock {
    my $self = shift;
    return IO::Socket::INET->new(PeerAddr => '127.0.0.1',
                                 PeerPort => $self->{udpport},
                                 Proto    => 'udp',
                                 LocalAddr => '127.0.0.1',
                                 LocalPort => MemcachedTest::free_port('udp'),
        );

}

1;
