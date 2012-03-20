# -*- Mode: cperl; cperl-indent-level: 4 -*-
package Test::Harness::Results;

use strict;
use vars qw($VERSION);
$VERSION = '0.01';

=head1 NAME

Test::Harness::Results - object for tracking results from a single test file

=head1 SYNOPSIS

One Test::Harness::Results object represents the results from one
test file getting analyzed.

=head1 CONSTRUCTION

=head2 new()

    my $results = new Test::Harness::Results;

Create a test point object.  Typically, however, you'll not create
one yourself, but access a Results object returned to you by
Test::Harness::Results.

=cut

sub new {
    my $class = shift;
    my $self  = bless {}, $class;

    return $self;
}

=head1 ACCESSORS

The following data points are defined:

  passing           true if the whole test is considered a pass 
                    (or skipped), false if its a failure

  exit              the exit code of the test run, if from a file
  wait              the wait code of the test run, if from a file

  max               total tests which should have been run
  seen              total tests actually seen
  skip_all          if the whole test was skipped, this will 
                      contain the reason.

  ok                number of tests which passed 
                      (including todo and skips)

  todo              number of todo tests seen
  bonus             number of todo tests which 
                      unexpectedly passed

  skip              number of tests skipped

So a successful test should have max == seen == ok.


There is one final item, the details.

  details           an array ref reporting the result of 
                    each test looks like this:

    $results{details}[$test_num - 1] = 
            { ok          => is the test considered ok?
              actual_ok   => did it literally say 'ok'?
              name        => name of the test (if any)
              diagnostics => test diagnostics (if any)
              type        => 'skip' or 'todo' (if any)
              reason      => reason for the above (if any)
            };

Element 0 of the details is test #1.  I tried it with element 1 being
#1 and 0 being empty, this is less awkward.


Each of the following fields has a getter and setter method.

=over 4

=item * wait

=item * exit

=cut

sub set_wait { my $self = shift; $self->{wait} = shift }
sub wait {
    my $self = shift;
    return $self->{wait} || 0;
}

sub set_skip_all { my $self = shift; $self->{skip_all} = shift }
sub skip_all {
    my $self = shift;
    return $self->{skip_all};
}

sub inc_max { my $self = shift; $self->{max} += (@_ ? shift : 1) }
sub max {
    my $self = shift;
    return $self->{max} || 0;
}

sub set_passing { my $self = shift; $self->{passing} = shift }
sub passing {
    my $self = shift;
    return $self->{passing} || 0;
}

sub inc_ok { my $self = shift; $self->{ok} += (@_ ? shift : 1) }
sub ok {
    my $self = shift;
    return $self->{ok} || 0;
}

sub set_exit { 
    my $self = shift; 
    if ($^O eq 'VMS') {
        eval {
            use vmsish q(status);
            $self->{exit} = shift;  # must be in same scope as pragma
        }
    }
    else {
        $self->{exit} = shift;
    }
}
sub exit {
    my $self = shift;
    return $self->{exit} || 0;
}

sub inc_bonus { my $self = shift; $self->{bonus}++ }
sub bonus {
    my $self = shift;
    return $self->{bonus} || 0;
}

sub set_skip_reason { my $self = shift; $self->{skip_reason} = shift }
sub skip_reason {
    my $self = shift;
    return $self->{skip_reason} || 0;
}

sub inc_skip { my $self = shift; $self->{skip}++ }
sub skip {
    my $self = shift;
    return $self->{skip} || 0;
}

sub inc_todo { my $self = shift; $self->{todo}++ }
sub todo {
    my $self = shift;
    return $self->{todo} || 0;
}

sub inc_seen { my $self = shift; $self->{seen}++ }
sub seen {
    my $self = shift;
    return $self->{seen} || 0;
}

sub set_details {
    my $self = shift;
    my $index = shift;
    my $details = shift;

    my $array = ($self->{details} ||= []);
    $array->[$index-1] = $details;
}

sub details {
    my $self = shift;
    return $self->{details} || [];
}

1;
