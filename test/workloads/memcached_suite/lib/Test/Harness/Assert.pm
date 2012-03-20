package Test::Harness::Assert;

use strict;
require Exporter;
use vars qw($VERSION @EXPORT @ISA);

$VERSION = '0.02';

@ISA = qw(Exporter);
@EXPORT = qw(assert);


=head1 NAME

Test::Harness::Assert - simple assert

=head1 SYNOPSIS

  ### FOR INTERNAL USE ONLY ###

  use Test::Harness::Assert;

  assert( EXPR, $name );

=head1 DESCRIPTION

A simple assert routine since we don't have Carp::Assert handy.

B<For internal use by Test::Harness ONLY!>

=head1 FUNCTIONS

=head2 C<assert()>

  assert( EXPR, $name );

If the expression is false the program aborts.

=cut

sub assert ($;$) {
    my($assert, $name) = @_;

    unless( $assert ) {
        require Carp;
        my $msg = 'Assert failed';
        $msg .= " - '$name'" if defined $name;
        $msg .= '!';
        Carp::croak($msg);
    }

}

=head1 AUTHOR

Michael G Schwern C<< <schwern at pobox.com> >>

=head1 SEE ALSO

L<Carp::Assert>

=cut

1;
