package Test::Builder::Tester;

use strict;
use vars qw(@EXPORT $VERSION @ISA);
$VERSION = "1.09";

use Test::Builder;
use Symbol;
use Carp;

=head1 NAME

Test::Builder::Tester - test testsuites that have been built with
Test::Builder

=head1 SYNOPSIS

    use Test::Builder::Tester tests => 1;
    use Test::More;

    test_out("not ok 1 - foo");
    test_fail(+1);
    fail("foo");
    test_test("fail works");

=head1 DESCRIPTION

A module that helps you test testing modules that are built with
B<Test::Builder>.

The testing system is designed to be used by performing a three step
process for each test you wish to test.  This process starts with using
C<test_out> and C<test_err> in advance to declare what the testsuite you
are testing will output with B<Test::Builder> to stdout and stderr.

You then can run the test(s) from your test suite that call
B<Test::Builder>.  At this point the output of B<Test::Builder> is
safely captured by B<Test::Builder::Tester> rather than being
interpreted as real test output.

The final stage is to call C<test_test> that will simply compare what you
predeclared to what B<Test::Builder> actually outputted, and report the
results back with a "ok" or "not ok" (with debugging) to the normal
output.

=cut

####
# set up testing
####

my $t = Test::Builder->new;

###
# make us an exporter
###

use Exporter;
@ISA = qw(Exporter);

@EXPORT = qw(test_out test_err test_fail test_diag test_test line_num);

# _export_to_level and import stolen directly from Test::More.  I am
# the king of cargo cult programming ;-)

# 5.004's Exporter doesn't have export_to_level.
sub _export_to_level
{
      my $pkg = shift;
      my $level = shift;
      (undef) = shift;                  # XXX redundant arg
      my $callpkg = caller($level);
      $pkg->export($callpkg, @_);
}

sub import {
    my $class = shift;
    my(@plan) = @_;

    my $caller = caller;

    $t->exported_to($caller);
    $t->plan(@plan);

    my @imports = ();
    foreach my $idx (0..$#plan) {
        if( $plan[$idx] eq 'import' ) {
            @imports = @{$plan[$idx+1]};
            last;
        }
    }

    __PACKAGE__->_export_to_level(1, __PACKAGE__, @imports);
}

###
# set up file handles
###

# create some private file handles
my $output_handle = gensym;
my $error_handle  = gensym;

# and tie them to this package
my $out = tie *$output_handle, "Test::Builder::Tester::Tie", "STDOUT";
my $err = tie *$error_handle,  "Test::Builder::Tester::Tie", "STDERR";

####
# exported functions
####

# for remembering that we're testing and where we're testing at
my $testing = 0;
my $testing_num;

# remembering where the file handles were originally connected
my $original_output_handle;
my $original_failure_handle;
my $original_todo_handle;

my $original_test_number;
my $original_harness_state;

my $original_harness_env;

# function that starts testing and redirects the filehandles for now
sub _start_testing
{
    # even if we're running under Test::Harness pretend we're not
    # for now.  This needed so Test::Builder doesn't add extra spaces
    $original_harness_env = $ENV{HARNESS_ACTIVE} || 0;
    $ENV{HARNESS_ACTIVE} = 0;

    # remember what the handles were set to
    $original_output_handle  = $t->output();
    $original_failure_handle = $t->failure_output();
    $original_todo_handle    = $t->todo_output();

    # switch out to our own handles
    $t->output($output_handle);
    $t->failure_output($error_handle);
    $t->todo_output($error_handle);

    # clear the expected list
    $out->reset();
    $err->reset();

    # remeber that we're testing
    $testing = 1;
    $testing_num = $t->current_test;
    $t->current_test(0);

    # look, we shouldn't do the ending stuff
    $t->no_ending(1);
}

=head2 Functions

These are the six methods that are exported as default.

=over 4

=item test_out

=item test_err

Procedures for predeclaring the output that your test suite is
expected to produce until C<test_test> is called.  These procedures
automatically assume that each line terminates with "\n".  So

   test_out("ok 1","ok 2");

is the same as

   test_out("ok 1\nok 2");

which is even the same as

   test_out("ok 1");
   test_out("ok 2");

Once C<test_out> or C<test_err> (or C<test_fail> or C<test_diag>) have
been called once all further output from B<Test::Builder> will be
captured by B<Test::Builder::Tester>.  This means that your will not
be able perform further tests to the normal output in the normal way
until you call C<test_test> (well, unless you manually meddle with the
output filehandles)

=cut

sub test_out(@)
{
    # do we need to do any setup?
    _start_testing() unless $testing;

    $out->expect(@_)
}

sub test_err(@)
{
    # do we need to do any setup?
    _start_testing() unless $testing;

    $err->expect(@_)
}

=item test_fail

Because the standard failure message that B<Test::Builder> produces
whenever a test fails will be a common occurrence in your test error
output, and because has changed between Test::Builder versions, rather
than forcing you to call C<test_err> with the string all the time like
so

    test_err("# Failed test ($0 at line ".line_num(+1).")");

C<test_fail> exists as a convenience function that can be called
instead.  It takes one argument, the offset from the current line that
the line that causes the fail is on.

    test_fail(+1);

This means that the example in the synopsis could be rewritten
more simply as:

   test_out("not ok 1 - foo");
   test_fail(+1);
   fail("foo");
   test_test("fail works");

=cut

sub test_fail
{
    # do we need to do any setup?
    _start_testing() unless $testing;

    # work out what line we should be on
    my ($package, $filename, $line) = caller;
    $line = $line + (shift() || 0); # prevent warnings

    # expect that on stderr
    $err->expect("#     Failed test ($0 at line $line)");
}

=item test_diag

As most of the remaining expected output to the error stream will be
created by Test::Builder's C<diag> function, B<Test::Builder::Tester>
provides a convience function C<test_diag> that you can use instead of
C<test_err>.

The C<test_diag> function prepends comment hashes and spacing to the
start and newlines to the end of the expected output passed to it and
adds it to the list of expected error output.  So, instead of writing

   test_err("# Couldn't open file");

you can write

   test_diag("Couldn't open file");

Remember that B<Test::Builder>'s diag function will not add newlines to
the end of output and test_diag will. So to check

   Test::Builder->new->diag("foo\n","bar\n");

You would do

  test_diag("foo","bar")

without the newlines.

=cut

sub test_diag
{
    # do we need to do any setup?
    _start_testing() unless $testing;

    # expect the same thing, but prepended with "#     "
    local $_;
    $err->expect(map {"# $_"} @_)
}

=item test_test

Actually performs the output check testing the tests, comparing the
data (with C<eq>) that we have captured from B<Test::Builder> against
that that was declared with C<test_out> and C<test_err>.

This takes name/value pairs that effect how the test is run.

=over

=item title (synonym 'name', 'label')

The name of the test that will be displayed after the C<ok> or C<not
ok>.

=item skip_out

Setting this to a true value will cause the test to ignore if the
output sent by the test to the output stream does not match that
declared with C<test_out>.

=item skip_err

Setting this to a true value will cause the test to ignore if the
output sent by the test to the error stream does not match that
declared with C<test_err>.

=back

As a convience, if only one argument is passed then this argument
is assumed to be the name of the test (as in the above examples.)

Once C<test_test> has been run test output will be redirected back to
the original filehandles that B<Test::Builder> was connected to
(probably STDOUT and STDERR,) meaning any further tests you run
will function normally and cause success/errors for B<Test::Harness>.

=cut

sub test_test
{
   # decode the arguements as described in the pod
   my $mess;
   my %args;
   if (@_ == 1)
     { $mess = shift }
   else
   {
     %args = @_;
     $mess = $args{name} if exists($args{name});
     $mess = $args{title} if exists($args{title});
     $mess = $args{label} if exists($args{label});
   }

    # er, are we testing?
    croak "Not testing.  You must declare output with a test function first."
	unless $testing;

    # okay, reconnect the test suite back to the saved handles
    $t->output($original_output_handle);
    $t->failure_output($original_failure_handle);
    $t->todo_output($original_todo_handle);

    # restore the test no, etc, back to the original point
    $t->current_test($testing_num);
    $testing = 0;

    # re-enable the original setting of the harness
    $ENV{HARNESS_ACTIVE} = $original_harness_env;

    # check the output we've stashed
    unless ($t->ok(    ($args{skip_out} || $out->check)
                    && ($args{skip_err} || $err->check),
                   $mess))
    {
      # print out the diagnostic information about why this
      # test failed

      local $_;

      $t->diag(map {"$_\n"} $out->complaint)
	unless $args{skip_out} || $out->check;

      $t->diag(map {"$_\n"} $err->complaint)
	unless $args{skip_err} || $err->check;
    }
}

=item line_num

A utility function that returns the line number that the function was
called on.  You can pass it an offset which will be added to the
result.  This is very useful for working out the correct text of
diagnostic functions that contain line numbers.

Essentially this is the same as the C<__LINE__> macro, but the
C<line_num(+3)> idiom is arguably nicer.

=cut

sub line_num
{
    my ($package, $filename, $line) = caller;
    return $line + (shift() || 0); # prevent warnings
}

=back

In addition to the six exported functions there there exists one
function that can only be accessed with a fully qualified function
call.

=over 4

=item color

When C<test_test> is called and the output that your tests generate
does not match that which you declared, C<test_test> will print out
debug information showing the two conflicting versions.  As this
output itself is debug information it can be confusing which part of
the output is from C<test_test> and which was the original output from
your original tests.  Also, it may be hard to spot things like
extraneous whitespace at the end of lines that may cause your test to
fail even though the output looks similar.

To assist you, if you have the B<Term::ANSIColor> module installed
(which you should do by default from perl 5.005 onwards), C<test_test>
can colour the background of the debug information to disambiguate the
different types of output. The debug output will have it's background
coloured green and red.  The green part represents the text which is
the same between the executed and actual output, the red shows which
part differs.

The C<color> function determines if colouring should occur or not.
Passing it a true or false value will enable or disable colouring
respectively, and the function called with no argument will return the
current setting.

To enable colouring from the command line, you can use the
B<Text::Builder::Tester::Color> module like so:

   perl -Mlib=Text::Builder::Tester::Color test.t

Or by including the B<Test::Builder::Tester::Color> module directly in
the PERL5LIB.

=cut

my $color;
sub color
{
  $color = shift if @_;
  $color;
}

=back

=head1 BUGS

Calls C<<Test::Builder->no_ending>> turning off the ending tests.
This is needed as otherwise it will trip out because we've run more
tests than we strictly should have and it'll register any failures we
had that we were testing for as real failures.

The color function doesn't work unless B<Term::ANSIColor> is installed
and is compatible with your terminal.

Bugs (and requests for new features) can be reported to the author
though the CPAN RT system:
L<http://rt.cpan.org/NoAuth/ReportBug.html?Queue=Test-Builder-Tester>

=head1 AUTHOR

Copyright Mark Fowler E<lt>mark@twoshortplanks.comE<gt> 2002, 2004.

Some code taken from B<Test::More> and B<Test::Catch>, written by by
Michael G Schwern E<lt>schwern@pobox.comE<gt>.  Hence, those parts
Copyright Micheal G Schwern 2001.  Used and distributed with
permission.

This program is free software; you can redistribute it
and/or modify it under the same terms as Perl itself.

=head1 NOTES

This code has been tested explicitly on the following versions
of perl: 5.7.3, 5.6.1, 5.6.0, 5.005_03, 5.004_05 and 5.004.

Thanks to Richard Clamp E<lt>richardc@unixbeard.netE<gt> for letting
me use his testing system to try this module out on.

=head1 SEE ALSO

L<Test::Builder>, L<Test::Builder::Tester::Color>, L<Test::More>.

=cut

1;

####################################################################
# Helper class that is used to remember expected and received data

package Test::Builder::Tester::Tie;

##
# add line(s) to be expected

sub expect
{
    my $self = shift;

    my @checks = @_;
    foreach my $check (@checks) {
        $check = $self->_translate_Failed_check($check);
        push @{$self->{wanted}}, ref $check ? $check : "$check\n";
    }
}


sub _translate_Failed_check
{
    my($self, $check) = @_;

    if( $check =~ /\A(.*)#     (Failed .*test) \((.*?) at line (\d+)\)\Z(?!\n)/ ) {
        $check = "/\Q$1\E#\\s+\Q$2\E.*?\\n?.*?\Qat $3\E line \Q$4\E.*\\n?/";
    }

    return $check;
}


##
# return true iff the expected data matches the got data

sub check
{
    my $self = shift;

    # turn off warnings as these might be undef
    local $^W = 0;

    my @checks = @{$self->{wanted}};
    my $got = $self->{got};
    foreach my $check (@checks) {
        $check = "\Q$check\E" unless ($check =~ s,^/(.*)/$,$1, or ref $check);
        return 0 unless $got =~ s/^$check//;
    }

    return length $got == 0;
}

##
# a complaint message about the inputs not matching (to be
# used for debugging messages)

sub complaint
{
    my $self = shift;
    my $type   = $self->type;
    my $got    = $self->got;
    my $wanted = join "\n", @{$self->wanted};

    # are we running in colour mode?
    if (Test::Builder::Tester::color)
    {
      # get color
      eval "require Term::ANSIColor";
      unless ($@)
      {
	# colours

	my $green = Term::ANSIColor::color("black").
	            Term::ANSIColor::color("on_green");
        my $red   = Term::ANSIColor::color("black").
                    Term::ANSIColor::color("on_red");
	my $reset = Term::ANSIColor::color("reset");

	# work out where the two strings start to differ
	my $char = 0;
	$char++ while substr($got, $char, 1) eq substr($wanted, $char, 1);

	# get the start string and the two end strings
	my $start     = $green . substr($wanted, 0,   $char);
	my $gotend    = $red   . substr($got   , $char) . $reset;
	my $wantedend = $red   . substr($wanted, $char) . $reset;

	# make the start turn green on and off
	$start =~ s/\n/$reset\n$green/g;

	# make the ends turn red on and off
	$gotend    =~ s/\n/$reset\n$red/g;
	$wantedend =~ s/\n/$reset\n$red/g;

	# rebuild the strings
	$got    = $start . $gotend;
	$wanted = $start . $wantedend;
      }
    }

    return "$type is:\n" .
           "$got\nnot:\n$wanted\nas expected"
}

##
# forget all expected and got data

sub reset
{
    my $self = shift;
    %$self = (
              type   => $self->{type},
              got    => '',
              wanted => [],
             );
}


sub got
{
    my $self = shift;
    return $self->{got};
}

sub wanted
{
    my $self = shift;
    return $self->{wanted};
}

sub type
{
    my $self = shift;
    return $self->{type};
}

###
# tie interface
###

sub PRINT  {
    my $self = shift;
    $self->{got} .= join '', @_;
}

sub TIEHANDLE {
    my($class, $type) = @_;

    my $self = bless {
                   type => $type
               }, $class;

    $self->reset;

    return $self;
}

sub READ {}
sub READLINE {}
sub GETC {}
sub FILENO {}

1;
