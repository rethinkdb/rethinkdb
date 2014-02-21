package Test::More;

use 5.004;

use strict;


# Can't use Carp because it might cause use_ok() to accidentally succeed
# even though the module being used forgot to use Carp.  Yes, this
# actually happened.
sub _carp {
    my($file, $line) = (caller(1))[1,2];
    warn @_, " at $file line $line\n";
}



use vars qw($VERSION @ISA @EXPORT %EXPORT_TAGS $TODO);
$VERSION = '0.72';
$VERSION = eval $VERSION;    # make the alpha version come out as a number

use Test::Builder::Module;
@ISA    = qw(Test::Builder::Module);
@EXPORT = qw(ok use_ok require_ok
             is isnt like unlike is_deeply
             cmp_ok
             skip todo todo_skip
             pass fail
             eq_array eq_hash eq_set
             $TODO
             plan
             can_ok  isa_ok
             diag
	     BAIL_OUT
            );


=head1 NAME

Test::More - yet another framework for writing test scripts

=head1 SYNOPSIS

  use Test::More tests => 23;
  # or
  use Test::More qw(no_plan);
  # or
  use Test::More skip_all => $reason;

  BEGIN { use_ok( 'Some::Module' ); }
  require_ok( 'Some::Module' );

  # Various ways to say "ok"
  ok($got eq $expected, $test_name);

  is  ($got, $expected, $test_name);
  isnt($got, $expected, $test_name);

  # Rather than print STDERR "# here's what went wrong\n"
  diag("here's what went wrong");

  like  ($got, qr/expected/, $test_name);
  unlike($got, qr/expected/, $test_name);

  cmp_ok($got, '==', $expected, $test_name);

  is_deeply($got_complex_structure, $expected_complex_structure, $test_name);

  SKIP: {
      skip $why, $how_many unless $have_some_feature;

      ok( foo(),       $test_name );
      is( foo(42), 23, $test_name );
  };

  TODO: {
      local $TODO = $why;

      ok( foo(),       $test_name );
      is( foo(42), 23, $test_name );
  };

  can_ok($module, @methods);
  isa_ok($object, $class);

  pass($test_name);
  fail($test_name);

  BAIL_OUT($why);

  # UNIMPLEMENTED!!!
  my @status = Test::More::status;


=head1 DESCRIPTION

B<STOP!> If you're just getting started writing tests, have a look at
Test::Simple first.  This is a drop in replacement for Test::Simple
which you can switch to once you get the hang of basic testing.

The purpose of this module is to provide a wide range of testing
utilities.  Various ways to say "ok" with better diagnostics,
facilities to skip tests, test future features and compare complicated
data structures.  While you can do almost anything with a simple
C<ok()> function, it doesn't provide good diagnostic output.


=head2 I love it when a plan comes together

Before anything else, you need a testing plan.  This basically declares
how many tests your script is going to run to protect against premature
failure.

The preferred way to do this is to declare a plan when you C<use Test::More>.

  use Test::More tests => 23;

There are rare cases when you will not know beforehand how many tests
your script is going to run.  In this case, you can declare that you
have no plan.  (Try to avoid using this as it weakens your test.)

  use Test::More qw(no_plan);

B<NOTE>: using no_plan requires a Test::Harness upgrade else it will
think everything has failed.  See L<CAVEATS and NOTES>).

In some cases, you'll want to completely skip an entire testing script.

  use Test::More skip_all => $skip_reason;

Your script will declare a skip with the reason why you skipped and
exit immediately with a zero (success).  See L<Test::Harness> for
details.

If you want to control what functions Test::More will export, you
have to use the 'import' option.  For example, to import everything
but 'fail', you'd do:

  use Test::More tests => 23, import => ['!fail'];

Alternatively, you can use the plan() function.  Useful for when you
have to calculate the number of tests.

  use Test::More;
  plan tests => keys %Stuff * 3;

or for deciding between running the tests at all:

  use Test::More;
  if( $^O eq 'MacOS' ) {
      plan skip_all => 'Test irrelevant on MacOS';
  }
  else {
      plan tests => 42;
  }

=cut

sub plan {
    my $tb = Test::More->builder;

    $tb->plan(@_);
}


# This implements "use Test::More 'no_diag'" but the behavior is
# deprecated.
sub import_extra {
    my $class = shift;
    my $list  = shift;

    my @other = ();
    my $idx = 0;
    while( $idx <= $#{$list} ) {
        my $item = $list->[$idx];

        if( defined $item and $item eq 'no_diag' ) {
            $class->builder->no_diag(1);
        }
        else {
            push @other, $item;
        }

        $idx++;
    }

    @$list = @other;
}


=head2 Test names

By convention, each test is assigned a number in order.  This is
largely done automatically for you.  However, it's often very useful to
assign a name to each test.  Which would you rather see:

  ok 4
  not ok 5
  ok 6

or

  ok 4 - basic multi-variable
  not ok 5 - simple exponential
  ok 6 - force == mass * acceleration

The later gives you some idea of what failed.  It also makes it easier
to find the test in your script, simply search for "simple
exponential".

All test functions take a name argument.  It's optional, but highly
suggested that you use it.


=head2 I'm ok, you're not ok.

The basic purpose of this module is to print out either "ok #" or "not
ok #" depending on if a given test succeeded or failed.  Everything
else is just gravy.

All of the following print "ok" or "not ok" depending on if the test
succeeded or failed.  They all also return true or false,
respectively.

=over 4

=item B<ok>

  ok($got eq $expected, $test_name);

This simply evaluates any expression (C<$got eq $expected> is just a
simple example) and uses that to determine if the test succeeded or
failed.  A true expression passes, a false one fails.  Very simple.

For example:

    ok( $exp{9} == 81,                   'simple exponential' );
    ok( Film->can('db_Main'),            'set_db()' );
    ok( $p->tests == 4,                  'saw tests' );
    ok( !grep !defined $_, @items,       'items populated' );

(Mnemonic:  "This is ok.")

$test_name is a very short description of the test that will be printed
out.  It makes it very easy to find a test in your script when it fails
and gives others an idea of your intentions.  $test_name is optional,
but we B<very> strongly encourage its use.

Should an ok() fail, it will produce some diagnostics:

    not ok 18 - sufficient mucus
    #   Failed test 'sufficient mucus'
    #   in foo.t at line 42.

This is the same as Test::Simple's ok() routine.

=cut

sub ok ($;$) {
    my($test, $name) = @_;
    my $tb = Test::More->builder;

    $tb->ok($test, $name);
}

=item B<is>

=item B<isnt>

  is  ( $got, $expected, $test_name );
  isnt( $got, $expected, $test_name );

Similar to ok(), is() and isnt() compare their two arguments
with C<eq> and C<ne> respectively and use the result of that to
determine if the test succeeded or failed.  So these:

    # Is the ultimate answer 42?
    is( ultimate_answer(), 42,          "Meaning of Life" );

    # $foo isn't empty
    isnt( $foo, '',     "Got some foo" );

are similar to these:

    ok( ultimate_answer() eq 42,        "Meaning of Life" );
    ok( $foo ne '',     "Got some foo" );

(Mnemonic:  "This is that."  "This isn't that.")

So why use these?  They produce better diagnostics on failure.  ok()
cannot know what you are testing for (beyond the name), but is() and
isnt() know what the test was and why it failed.  For example this
test:

    my $foo = 'waffle';  my $bar = 'yarblokos';
    is( $foo, $bar,   'Is foo the same as bar?' );

Will produce something like this:

    not ok 17 - Is foo the same as bar?
    #   Failed test 'Is foo the same as bar?'
    #   in foo.t at line 139.
    #          got: 'waffle'
    #     expected: 'yarblokos'

So you can figure out what went wrong without rerunning the test.

You are encouraged to use is() and isnt() over ok() where possible,
however do not be tempted to use them to find out if something is
true or false!

  # XXX BAD!
  is( exists $brooklyn{tree}, 1, 'A tree grows in Brooklyn' );

This does not check if C<exists $brooklyn{tree}> is true, it checks if
it returns 1.  Very different.  Similar caveats exist for false and 0.
In these cases, use ok().

  ok( exists $brooklyn{tree},    'A tree grows in Brooklyn' );

For those grammatical pedants out there, there's an C<isn't()>
function which is an alias of isnt().

=cut

sub is ($$;$) {
    my $tb = Test::More->builder;

    $tb->is_eq(@_);
}

sub isnt ($$;$) {
    my $tb = Test::More->builder;

    $tb->isnt_eq(@_);
}

*isn't = \&isnt;


=item B<like>

  like( $got, qr/expected/, $test_name );

Similar to ok(), like() matches $got against the regex C<qr/expected/>.

So this:

    like($got, qr/expected/, 'this is like that');

is similar to:

    ok( $got =~ /expected/, 'this is like that');

(Mnemonic "This is like that".)

The second argument is a regular expression.  It may be given as a
regex reference (i.e. C<qr//>) or (for better compatibility with older
perls) as a string that looks like a regex (alternative delimiters are
currently not supported):

    like( $got, '/expected/', 'this is like that' );

Regex options may be placed on the end (C<'/expected/i'>).

Its advantages over ok() are similar to that of is() and isnt().  Better
diagnostics on failure.

=cut

sub like ($$;$) {
    my $tb = Test::More->builder;

    $tb->like(@_);
}


=item B<unlike>

  unlike( $got, qr/expected/, $test_name );

Works exactly as like(), only it checks if $got B<does not> match the
given pattern.

=cut

sub unlike ($$;$) {
    my $tb = Test::More->builder;

    $tb->unlike(@_);
}


=item B<cmp_ok>

  cmp_ok( $got, $op, $expected, $test_name );

Halfway between ok() and is() lies cmp_ok().  This allows you to
compare two arguments using any binary perl operator.

    # ok( $got eq $expected );
    cmp_ok( $got, 'eq', $expected, 'this eq that' );

    # ok( $got == $expected );
    cmp_ok( $got, '==', $expected, 'this == that' );

    # ok( $got && $expected );
    cmp_ok( $got, '&&', $expected, 'this && that' );
    ...etc...

Its advantage over ok() is when the test fails you'll know what $got
and $expected were:

    not ok 1
    #   Failed test in foo.t at line 12.
    #     '23'
    #         &&
    #     undef

It's also useful in those cases where you are comparing numbers and
is()'s use of C<eq> will interfere:

    cmp_ok( $big_hairy_number, '==', $another_big_hairy_number );

=cut

sub cmp_ok($$$;$) {
    my $tb = Test::More->builder;

    $tb->cmp_ok(@_);
}


=item B<can_ok>

  can_ok($module, @methods);
  can_ok($object, @methods);

Checks to make sure the $module or $object can do these @methods
(works with functions, too).

    can_ok('Foo', qw(this that whatever));

is almost exactly like saying:

    ok( Foo->can('this') && 
        Foo->can('that') && 
        Foo->can('whatever') 
      );

only without all the typing and with a better interface.  Handy for
quickly testing an interface.

No matter how many @methods you check, a single can_ok() call counts
as one test.  If you desire otherwise, use:

    foreach my $meth (@methods) {
        can_ok('Foo', $meth);
    }

=cut

sub can_ok ($@) {
    my($proto, @methods) = @_;
    my $class = ref $proto || $proto;
    my $tb = Test::More->builder;

    unless( $class ) {
        my $ok = $tb->ok( 0, "->can(...)" );
        $tb->diag('    can_ok() called with empty class or reference');
        return $ok;
    }

    unless( @methods ) {
        my $ok = $tb->ok( 0, "$class->can(...)" );
        $tb->diag('    can_ok() called with no methods');
        return $ok;
    }

    my @nok = ();
    foreach my $method (@methods) {
        $tb->_try(sub { $proto->can($method) }) or push @nok, $method;
    }

    my $name;
    $name = @methods == 1 ? "$class->can('$methods[0]')" 
                          : "$class->can(...)";

    my $ok = $tb->ok( !@nok, $name );

    $tb->diag(map "    $class->can('$_') failed\n", @nok);

    return $ok;
}

=item B<isa_ok>

  isa_ok($object, $class, $object_name);
  isa_ok($ref,    $type,  $ref_name);

Checks to see if the given C<< $object->isa($class) >>.  Also checks to make
sure the object was defined in the first place.  Handy for this sort
of thing:

    my $obj = Some::Module->new;
    isa_ok( $obj, 'Some::Module' );

where you'd otherwise have to write

    my $obj = Some::Module->new;
    ok( defined $obj && $obj->isa('Some::Module') );

to safeguard against your test script blowing up.

It works on references, too:

    isa_ok( $array_ref, 'ARRAY' );

The diagnostics of this test normally just refer to 'the object'.  If
you'd like them to be more specific, you can supply an $object_name
(for example 'Test customer').

=cut

sub isa_ok ($$;$) {
    my($object, $class, $obj_name) = @_;
    my $tb = Test::More->builder;

    my $diag;
    $obj_name = 'The object' unless defined $obj_name;
    my $name = "$obj_name isa $class";
    if( !defined $object ) {
        $diag = "$obj_name isn't defined";
    }
    elsif( !ref $object ) {
        $diag = "$obj_name isn't a reference";
    }
    else {
        # We can't use UNIVERSAL::isa because we want to honor isa() overrides
        my($rslt, $error) = $tb->_try(sub { $object->isa($class) });
        if( $error ) {
            if( $error =~ /^Can't call method "isa" on unblessed reference/ ) {
                # Its an unblessed reference
                if( !UNIVERSAL::isa($object, $class) ) {
                    my $ref = ref $object;
                    $diag = "$obj_name isn't a '$class' it's a '$ref'";
                }
            } else {
                die <<WHOA;
WHOA! I tried to call ->isa on your object and got some weird error.
Here's the error.
$error
WHOA
            }
        }
        elsif( !$rslt ) {
            my $ref = ref $object;
            $diag = "$obj_name isn't a '$class' it's a '$ref'";
        }
    }
            
      

    my $ok;
    if( $diag ) {
        $ok = $tb->ok( 0, $name );
        $tb->diag("    $diag\n");
    }
    else {
        $ok = $tb->ok( 1, $name );
    }

    return $ok;
}


=item B<pass>

=item B<fail>

  pass($test_name);
  fail($test_name);

Sometimes you just want to say that the tests have passed.  Usually
the case is you've got some complicated condition that is difficult to
wedge into an ok().  In this case, you can simply use pass() (to
declare the test ok) or fail (for not ok).  They are synonyms for
ok(1) and ok(0).

Use these very, very, very sparingly.

=cut

sub pass (;$) {
    my $tb = Test::More->builder;
    $tb->ok(1, @_);
}

sub fail (;$) {
    my $tb = Test::More->builder;
    $tb->ok(0, @_);
}

=back


=head2 Module tests

You usually want to test if the module you're testing loads ok, rather
than just vomiting if its load fails.  For such purposes we have
C<use_ok> and C<require_ok>.

=over 4

=item B<use_ok>

   BEGIN { use_ok($module); }
   BEGIN { use_ok($module, @imports); }

These simply use the given $module and test to make sure the load
happened ok.  It's recommended that you run use_ok() inside a BEGIN
block so its functions are exported at compile-time and prototypes are
properly honored.

If @imports are given, they are passed through to the use.  So this:

   BEGIN { use_ok('Some::Module', qw(foo bar)) }

is like doing this:

   use Some::Module qw(foo bar);

Version numbers can be checked like so:

   # Just like "use Some::Module 1.02"
   BEGIN { use_ok('Some::Module', 1.02) }

Don't try to do this:

   BEGIN {
       use_ok('Some::Module');

       ...some code that depends on the use...
       ...happening at compile time...
   }

because the notion of "compile-time" is relative.  Instead, you want:

  BEGIN { use_ok('Some::Module') }
  BEGIN { ...some code that depends on the use... }


=cut

sub use_ok ($;@) {
    my($module, @imports) = @_;
    @imports = () unless @imports;
    my $tb = Test::More->builder;

    my($pack,$filename,$line) = caller;

    local($@,$!,$SIG{__DIE__});   # isolate eval

    if( @imports == 1 and $imports[0] =~ /^\d+(?:\.\d+)?$/ ) {
        # probably a version check.  Perl needs to see the bare number
        # for it to work with non-Exporter based modules.
        eval <<USE;
package $pack;
use $module $imports[0];
USE
    }
    else {
        eval <<USE;
package $pack;
use $module \@imports;
USE
    }

    my $ok = $tb->ok( !$@, "use $module;" );

    unless( $ok ) {
        chomp $@;
        $@ =~ s{^BEGIN failed--compilation aborted at .*$}
                {BEGIN failed--compilation aborted at $filename line $line.}m;
        $tb->diag(<<DIAGNOSTIC);
    Tried to use '$module'.
    Error:  $@
DIAGNOSTIC

    }

    return $ok;
}

=item B<require_ok>

   require_ok($module);
   require_ok($file);

Like use_ok(), except it requires the $module or $file.

=cut

sub require_ok ($) {
    my($module) = shift;
    my $tb = Test::More->builder;

    my $pack = caller;

    # Try to deterine if we've been given a module name or file.
    # Module names must be barewords, files not.
    $module = qq['$module'] unless _is_module_name($module);

    local($!, $@, $SIG{__DIE__}); # isolate eval
    local $SIG{__DIE__};
    eval <<REQUIRE;
package $pack;
require $module;
REQUIRE

    my $ok = $tb->ok( !$@, "require $module;" );

    unless( $ok ) {
        chomp $@;
        $tb->diag(<<DIAGNOSTIC);
    Tried to require '$module'.
    Error:  $@
DIAGNOSTIC

    }

    return $ok;
}


sub _is_module_name {
    my $module = shift;

    # Module names start with a letter.
    # End with an alphanumeric.
    # The rest is an alphanumeric or ::
    $module =~ s/\b::\b//g;
    $module =~ /^[a-zA-Z]\w*$/;
}

=back


=head2 Complex data structures

Not everything is a simple eq check or regex.  There are times you
need to see if two data structures are equivalent.  For these
instances Test::More provides a handful of useful functions.

B<NOTE> I'm not quite sure what will happen with filehandles.

=over 4

=item B<is_deeply>

  is_deeply( $got, $expected, $test_name );

Similar to is(), except that if $got and $expected are references, it
does a deep comparison walking each data structure to see if they are
equivalent.  If the two structures are different, it will display the
place where they start differing.

is_deeply() compares the dereferenced values of references, the
references themselves (except for their type) are ignored.  This means
aspects such as blessing and ties are not considered "different".

is_deeply() current has very limited handling of function reference
and globs.  It merely checks if they have the same referent.  This may
improve in the future.

Test::Differences and Test::Deep provide more in-depth functionality
along these lines.

=cut

use vars qw(@Data_Stack %Refs_Seen);
my $DNE = bless [], 'Does::Not::Exist';

sub _dne {
    ref $_[0] eq ref $DNE;
}


sub is_deeply {
    my $tb = Test::More->builder;

    unless( @_ == 2 or @_ == 3 ) {
        my $msg = <<WARNING;
is_deeply() takes two or three args, you gave %d.
This usually means you passed an array or hash instead 
of a reference to it
WARNING
        chop $msg;   # clip off newline so carp() will put in line/file

        _carp sprintf $msg, scalar @_;

	return $tb->ok(0);
    }

    my($got, $expected, $name) = @_;

    $tb->_unoverload_str(\$expected, \$got);

    my $ok;
    if( !ref $got and !ref $expected ) {  		# neither is a reference
        $ok = $tb->is_eq($got, $expected, $name);
    }
    elsif( !ref $got xor !ref $expected ) {  	# one's a reference, one isn't
        $ok = $tb->ok(0, $name);
	$tb->diag( _format_stack({ vals => [ $got, $expected ] }) );
    }
    else {			       		# both references
        local @Data_Stack = ();
        if( _deep_check($got, $expected) ) {
            $ok = $tb->ok(1, $name);
        }
        else {
            $ok = $tb->ok(0, $name);
            $tb->diag(_format_stack(@Data_Stack));
        }
    }

    return $ok;
}

sub _format_stack {
    my(@Stack) = @_;

    my $var = '$FOO';
    my $did_arrow = 0;
    foreach my $entry (@Stack) {
        my $type = $entry->{type} || '';
        my $idx  = $entry->{'idx'};
        if( $type eq 'HASH' ) {
            $var .= "->" unless $did_arrow++;
            $var .= "{$idx}";
        }
        elsif( $type eq 'ARRAY' ) {
            $var .= "->" unless $did_arrow++;
            $var .= "[$idx]";
        }
        elsif( $type eq 'REF' ) {
            $var = "\${$var}";
        }
    }

    my @vals = @{$Stack[-1]{vals}}[0,1];
    my @vars = ();
    ($vars[0] = $var) =~ s/\$FOO/     \$got/;
    ($vars[1] = $var) =~ s/\$FOO/\$expected/;

    my $out = "Structures begin differing at:\n";
    foreach my $idx (0..$#vals) {
        my $val = $vals[$idx];
        $vals[$idx] = !defined $val ? 'undef'          :
                      _dne($val)    ? "Does not exist" :
                      ref $val      ? "$val"           :
                                      "'$val'";
    }

    $out .= "$vars[0] = $vals[0]\n";
    $out .= "$vars[1] = $vals[1]\n";

    $out =~ s/^/    /msg;
    return $out;
}


sub _type {
    my $thing = shift;

    return '' if !ref $thing;

    for my $type (qw(ARRAY HASH REF SCALAR GLOB CODE Regexp)) {
        return $type if UNIVERSAL::isa($thing, $type);
    }

    return '';
}

=back


=head2 Diagnostics

If you pick the right test function, you'll usually get a good idea of
what went wrong when it failed.  But sometimes it doesn't work out
that way.  So here we have ways for you to write your own diagnostic
messages which are safer than just C<print STDERR>.

=over 4

=item B<diag>

  diag(@diagnostic_message);

Prints a diagnostic message which is guaranteed not to interfere with
test output.  Like C<print> @diagnostic_message is simply concatenated
together.

Handy for this sort of thing:

    ok( grep(/foo/, @users), "There's a foo user" ) or
        diag("Since there's no foo, check that /etc/bar is set up right");

which would produce:

    not ok 42 - There's a foo user
    #   Failed test 'There's a foo user'
    #   in foo.t at line 52.
    # Since there's no foo, check that /etc/bar is set up right.

You might remember C<ok() or diag()> with the mnemonic C<open() or
die()>.

B<NOTE> The exact formatting of the diagnostic output is still
changing, but it is guaranteed that whatever you throw at it it won't
interfere with the test.

=cut

sub diag {
    my $tb = Test::More->builder;

    $tb->diag(@_);
}


=back


=head2 Conditional tests

Sometimes running a test under certain conditions will cause the
test script to die.  A certain function or method isn't implemented
(such as fork() on MacOS), some resource isn't available (like a 
net connection) or a module isn't available.  In these cases it's
necessary to skip tests, or declare that they are supposed to fail
but will work in the future (a todo test).

For more details on the mechanics of skip and todo tests see
L<Test::Harness>.

The way Test::More handles this is with a named block.  Basically, a
block of tests which can be skipped over or made todo.  It's best if I
just show you...

=over 4

=item B<SKIP: BLOCK>

  SKIP: {
      skip $why, $how_many if $condition;

      ...normal testing code goes here...
  }

This declares a block of tests that might be skipped, $how_many tests
there are, $why and under what $condition to skip them.  An example is
the easiest way to illustrate:

    SKIP: {
        eval { require HTML::Lint };

        skip "HTML::Lint not installed", 2 if $@;

        my $lint = new HTML::Lint;
        isa_ok( $lint, "HTML::Lint" );

        $lint->parse( $html );
        is( $lint->errors, 0, "No errors found in HTML" );
    }

If the user does not have HTML::Lint installed, the whole block of
code I<won't be run at all>.  Test::More will output special ok's
which Test::Harness interprets as skipped, but passing, tests.

It's important that $how_many accurately reflects the number of tests
in the SKIP block so the # of tests run will match up with your plan.
If your plan is C<no_plan> $how_many is optional and will default to 1.

It's perfectly safe to nest SKIP blocks.  Each SKIP block must have
the label C<SKIP>, or Test::More can't work its magic.

You don't skip tests which are failing because there's a bug in your
program, or for which you don't yet have code written.  For that you
use TODO.  Read on.

=cut

#'#
sub skip {
    my($why, $how_many) = @_;
    my $tb = Test::More->builder;

    unless( defined $how_many ) {
        # $how_many can only be avoided when no_plan is in use.
        _carp "skip() needs to know \$how_many tests are in the block"
          unless $tb->has_plan eq 'no_plan';
        $how_many = 1;
    }

    if( defined $how_many and $how_many =~ /\D/ ) {
        _carp "skip() was passed a non-numeric number of tests.  Did you get the arguments backwards?";
        $how_many = 1;
    }

    for( 1..$how_many ) {
        $tb->skip($why);
    }

    local $^W = 0;
    last SKIP;
}


=item B<TODO: BLOCK>

    TODO: {
        local $TODO = $why if $condition;

        ...normal testing code goes here...
    }

Declares a block of tests you expect to fail and $why.  Perhaps it's
because you haven't fixed a bug or haven't finished a new feature:

    TODO: {
        local $TODO = "URI::Geller not finished";

        my $card = "Eight of clubs";
        is( URI::Geller->your_card, $card, 'Is THIS your card?' );

        my $spoon;
        URI::Geller->bend_spoon;
        is( $spoon, 'bent',    "Spoon bending, that's original" );
    }

With a todo block, the tests inside are expected to fail.  Test::More
will run the tests normally, but print out special flags indicating
they are "todo".  Test::Harness will interpret failures as being ok.
Should anything succeed, it will report it as an unexpected success.
You then know the thing you had todo is done and can remove the
TODO flag.

The nice part about todo tests, as opposed to simply commenting out a
block of tests, is it's like having a programmatic todo list.  You know
how much work is left to be done, you're aware of what bugs there are,
and you'll know immediately when they're fixed.

Once a todo test starts succeeding, simply move it outside the block.
When the block is empty, delete it.

B<NOTE>: TODO tests require a Test::Harness upgrade else it will
treat it as a normal failure.  See L<CAVEATS and NOTES>).


=item B<todo_skip>

    TODO: {
        todo_skip $why, $how_many if $condition;

        ...normal testing code...
    }

With todo tests, it's best to have the tests actually run.  That way
you'll know when they start passing.  Sometimes this isn't possible.
Often a failing test will cause the whole program to die or hang, even
inside an C<eval BLOCK> with and using C<alarm>.  In these extreme
cases you have no choice but to skip over the broken tests entirely.

The syntax and behavior is similar to a C<SKIP: BLOCK> except the
tests will be marked as failing but todo.  Test::Harness will
interpret them as passing.

=cut

sub todo_skip {
    my($why, $how_many) = @_;
    my $tb = Test::More->builder;

    unless( defined $how_many ) {
        # $how_many can only be avoided when no_plan is in use.
        _carp "todo_skip() needs to know \$how_many tests are in the block"
          unless $tb->has_plan eq 'no_plan';
        $how_many = 1;
    }

    for( 1..$how_many ) {
        $tb->todo_skip($why);
    }

    local $^W = 0;
    last TODO;
}

=item When do I use SKIP vs. TODO?

B<If it's something the user might not be able to do>, use SKIP.
This includes optional modules that aren't installed, running under
an OS that doesn't have some feature (like fork() or symlinks), or maybe
you need an Internet connection and one isn't available.

B<If it's something the programmer hasn't done yet>, use TODO.  This
is for any code you haven't written yet, or bugs you have yet to fix,
but want to put tests in your testing script (always a good idea).


=back


=head2 Test control

=over 4

=item B<BAIL_OUT>

    BAIL_OUT($reason);

Indicates to the harness that things are going so badly all testing
should terminate.  This includes the running any additional test scripts.

This is typically used when testing cannot continue such as a critical
module failing to compile or a necessary external utility not being
available such as a database connection failing.

The test will exit with 255.

=cut

sub BAIL_OUT {
    my $reason = shift;
    my $tb = Test::More->builder;

    $tb->BAIL_OUT($reason);
}

=back


=head2 Discouraged comparison functions

The use of the following functions is discouraged as they are not
actually testing functions and produce no diagnostics to help figure
out what went wrong.  They were written before is_deeply() existed
because I couldn't figure out how to display a useful diff of two
arbitrary data structures.

These functions are usually used inside an ok().

    ok( eq_array(\@got, \@expected) );

C<is_deeply()> can do that better and with diagnostics.  

    is_deeply( \@got, \@expected );

They may be deprecated in future versions.

=over 4

=item B<eq_array>

  my $is_eq = eq_array(\@got, \@expected);

Checks if two arrays are equivalent.  This is a deep check, so
multi-level structures are handled correctly.

=cut

#'#
sub eq_array {
    local @Data_Stack;
    _deep_check(@_);
}

sub _eq_array  {
    my($a1, $a2) = @_;

    if( grep !_type($_) eq 'ARRAY', $a1, $a2 ) {
        warn "eq_array passed a non-array ref";
        return 0;
    }

    return 1 if $a1 eq $a2;

    my $ok = 1;
    my $max = $#$a1 > $#$a2 ? $#$a1 : $#$a2;
    for (0..$max) {
        my $e1 = $_ > $#$a1 ? $DNE : $a1->[$_];
        my $e2 = $_ > $#$a2 ? $DNE : $a2->[$_];

        push @Data_Stack, { type => 'ARRAY', idx => $_, vals => [$e1, $e2] };
        $ok = _deep_check($e1,$e2);
        pop @Data_Stack if $ok;

        last unless $ok;
    }

    return $ok;
}

sub _deep_check {
    my($e1, $e2) = @_;
    my $tb = Test::More->builder;

    my $ok = 0;

    # Effectively turn %Refs_Seen into a stack.  This avoids picking up
    # the same referenced used twice (such as [\$a, \$a]) to be considered
    # circular.
    local %Refs_Seen = %Refs_Seen;

    {
        # Quiet uninitialized value warnings when comparing undefs.
        local $^W = 0; 

        $tb->_unoverload_str(\$e1, \$e2);

        # Either they're both references or both not.
        my $same_ref = !(!ref $e1 xor !ref $e2);
	my $not_ref  = (!ref $e1 and !ref $e2);

        if( defined $e1 xor defined $e2 ) {
            $ok = 0;
        }
        elsif ( _dne($e1) xor _dne($e2) ) {
            $ok = 0;
        }
        elsif ( $same_ref and ($e1 eq $e2) ) {
            $ok = 1;
        }
	elsif ( $not_ref ) {
	    push @Data_Stack, { type => '', vals => [$e1, $e2] };
	    $ok = 0;
	}
        else {
            if( $Refs_Seen{$e1} ) {
                return $Refs_Seen{$e1} eq $e2;
            }
            else {
                $Refs_Seen{$e1} = "$e2";
            }

            my $type = _type($e1);
            $type = 'DIFFERENT' unless _type($e2) eq $type;

            if( $type eq 'DIFFERENT' ) {
                push @Data_Stack, { type => $type, vals => [$e1, $e2] };
                $ok = 0;
            }
            elsif( $type eq 'ARRAY' ) {
                $ok = _eq_array($e1, $e2);
            }
            elsif( $type eq 'HASH' ) {
                $ok = _eq_hash($e1, $e2);
            }
            elsif( $type eq 'REF' ) {
                push @Data_Stack, { type => $type, vals => [$e1, $e2] };
                $ok = _deep_check($$e1, $$e2);
                pop @Data_Stack if $ok;
            }
            elsif( $type eq 'SCALAR' ) {
                push @Data_Stack, { type => 'REF', vals => [$e1, $e2] };
                $ok = _deep_check($$e1, $$e2);
                pop @Data_Stack if $ok;
            }
            elsif( $type ) {
                push @Data_Stack, { type => $type, vals => [$e1, $e2] };
                $ok = 0;
            }
	    else {
		_whoa(1, "No type in _deep_check");
	    }
        }
    }

    return $ok;
}


sub _whoa {
    my($check, $desc) = @_;
    if( $check ) {
        die <<WHOA;
WHOA!  $desc
This should never happen!  Please contact the author immediately!
WHOA
    }
}


=item B<eq_hash>

  my $is_eq = eq_hash(\%got, \%expected);

Determines if the two hashes contain the same keys and values.  This
is a deep check.

=cut

sub eq_hash {
    local @Data_Stack;
    return _deep_check(@_);
}

sub _eq_hash {
    my($a1, $a2) = @_;

    if( grep !_type($_) eq 'HASH', $a1, $a2 ) {
        warn "eq_hash passed a non-hash ref";
        return 0;
    }

    return 1 if $a1 eq $a2;

    my $ok = 1;
    my $bigger = keys %$a1 > keys %$a2 ? $a1 : $a2;
    foreach my $k (keys %$bigger) {
        my $e1 = exists $a1->{$k} ? $a1->{$k} : $DNE;
        my $e2 = exists $a2->{$k} ? $a2->{$k} : $DNE;

        push @Data_Stack, { type => 'HASH', idx => $k, vals => [$e1, $e2] };
        $ok = _deep_check($e1, $e2);
        pop @Data_Stack if $ok;

        last unless $ok;
    }

    return $ok;
}

=item B<eq_set>

  my $is_eq = eq_set(\@got, \@expected);

Similar to eq_array(), except the order of the elements is B<not>
important.  This is a deep check, but the irrelevancy of order only
applies to the top level.

    ok( eq_set(\@got, \@expected) );

Is better written:

    is_deeply( [sort @got], [sort @expected] );

B<NOTE> By historical accident, this is not a true set comparison.
While the order of elements does not matter, duplicate elements do.

B<NOTE> eq_set() does not know how to deal with references at the top
level.  The following is an example of a comparison which might not work:

    eq_set([\1, \2], [\2, \1]);

Test::Deep contains much better set comparison functions.

=cut

sub eq_set  {
    my($a1, $a2) = @_;
    return 0 unless @$a1 == @$a2;

    # There's faster ways to do this, but this is easiest.
    local $^W = 0;

    # It really doesn't matter how we sort them, as long as both arrays are 
    # sorted with the same algorithm.
    #
    # Ensure that references are not accidentally treated the same as a
    # string containing the reference.
    #
    # Have to inline the sort routine due to a threading/sort bug.
    # See [rt.cpan.org 6782]
    #
    # I don't know how references would be sorted so we just don't sort
    # them.  This means eq_set doesn't really work with refs.
    return eq_array(
           [grep(ref, @$a1), sort( grep(!ref, @$a1) )],
           [grep(ref, @$a2), sort( grep(!ref, @$a2) )],
    );
}

=back


=head2 Extending and Embedding Test::More

Sometimes the Test::More interface isn't quite enough.  Fortunately,
Test::More is built on top of Test::Builder which provides a single,
unified backend for any test library to use.  This means two test
libraries which both use Test::Builder B<can be used together in the
same program>.

If you simply want to do a little tweaking of how the tests behave,
you can access the underlying Test::Builder object like so:

=over 4

=item B<builder>

    my $test_builder = Test::More->builder;

Returns the Test::Builder object underlying Test::More for you to play
with.


=back


=head1 EXIT CODES

If all your tests passed, Test::Builder will exit with zero (which is
normal).  If anything failed it will exit with how many failed.  If
you run less (or more) tests than you planned, the missing (or extras)
will be considered failures.  If no tests were ever run Test::Builder
will throw a warning and exit with 255.  If the test died, even after
having successfully completed all its tests, it will still be
considered a failure and will exit with 255.

So the exit codes are...

    0                   all tests successful
    255                 test died or all passed but wrong # of tests run
    any other number    how many failed (including missing or extras)

If you fail more than 254 tests, it will be reported as 254.

B<NOTE>  This behavior may go away in future versions.


=head1 CAVEATS and NOTES

=over 4

=item Backwards compatibility

Test::More works with Perls as old as 5.004_05.


=item Overloaded objects

String overloaded objects are compared B<as strings> (or in cmp_ok()'s
case, strings or numbers as appropriate to the comparison op).  This
prevents Test::More from piercing an object's interface allowing
better blackbox testing.  So if a function starts returning overloaded
objects instead of bare strings your tests won't notice the
difference.  This is good.

However, it does mean that functions like is_deeply() cannot be used to
test the internals of string overloaded objects.  In this case I would
suggest Test::Deep which contains more flexible testing functions for
complex data structures.


=item Threads

Test::More will only be aware of threads if "use threads" has been done
I<before> Test::More is loaded.  This is ok:

    use threads;
    use Test::More;

This may cause problems:

    use Test::More
    use threads;

5.8.1 and above are supported.  Anything below that has too many bugs.


=item Test::Harness upgrade

no_plan and todo depend on new Test::Harness features and fixes.  If
you're going to distribute tests that use no_plan or todo your
end-users will have to upgrade Test::Harness to the latest one on
CPAN.  If you avoid no_plan and TODO tests, the stock Test::Harness
will work fine.

Installing Test::More should also upgrade Test::Harness.

=back


=head1 HISTORY

This is a case of convergent evolution with Joshua Pritikin's Test
module.  I was largely unaware of its existence when I'd first
written my own ok() routines.  This module exists because I can't
figure out how to easily wedge test names into Test's interface (along
with a few other problems).

The goal here is to have a testing utility that's simple to learn,
quick to use and difficult to trip yourself up with while still
providing more flexibility than the existing Test.pm.  As such, the
names of the most common routines are kept tiny, special cases and
magic side-effects are kept to a minimum.  WYSIWYG.


=head1 SEE ALSO

L<Test::Simple> if all this confuses you and you just want to write
some tests.  You can upgrade to Test::More later (it's forward
compatible).

L<Test> is the old testing module.  Its main benefit is that it has
been distributed with Perl since 5.004_05.

L<Test::Harness> for details on how your test results are interpreted
by Perl.

L<Test::Differences> for more ways to test complex data structures.
And it plays well with Test::More.

L<Test::Class> is like XUnit but more perlish.

L<Test::Deep> gives you more powerful complex data structure testing.

L<Test::Unit> is XUnit style testing.

L<Test::Inline> shows the idea of embedded testing.

L<Bundle::Test> installs a whole bunch of useful test modules.


=head1 AUTHORS

Michael G Schwern E<lt>schwern@pobox.comE<gt> with much inspiration
from Joshua Pritikin's Test module and lots of help from Barrie
Slaymaker, Tony Bowden, blackstar.co.uk, chromatic, Fergal Daly and
the perl-qa gang.


=head1 BUGS

See F<http://rt.cpan.org> to report and view bugs.


=head1 COPYRIGHT

Copyright 2001-2002, 2004-2006 by Michael G Schwern E<lt>schwern@pobox.comE<gt>.

This program is free software; you can redistribute it and/or
modify it under the same terms as Perl itself.

See F<http://www.perl.com/perl/misc/Artistic.html>

=cut

1;
