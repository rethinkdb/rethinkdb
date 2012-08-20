package Test::Builder::Tester::Color;

use strict;

require Test::Builder::Tester;

=head1 NAME

Test::Builder::Tester::Color - turn on colour in Test::Builder::Tester

=head1 SYNOPSIS

   When running a test script

     perl -MTest::Builder::Tester::Color test.t

=head1 DESCRIPTION

Importing this module causes the subroutine color in Test::Builder::Tester
to be called with a true value causing colour highlighting to be turned
on in debug output.

The sole purpose of this module is to enable colour highlighting
from the command line.

=cut

sub import
{
    Test::Builder::Tester::color(1);
}

=head1 AUTHOR

Copyright Mark Fowler E<lt>mark@twoshortplanks.comE<gt> 2002.

This program is free software; you can redistribute it
and/or modify it under the same terms as Perl itself.

=head1 BUGS

This module will have no effect unless Term::ANSIColor is installed.

=head1 SEE ALSO

L<Test::Builder::Tester>, L<Term::ANSIColor>

=cut

1;
