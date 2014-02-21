# -*- Mode: cperl; cperl-indent-level: 4 -*-
package Test::Harness::Straps;

use strict;
use vars qw($VERSION);
$VERSION = '0.26_01';

use Config;
use Test::Harness::Assert;
use Test::Harness::Iterator;
use Test::Harness::Point;
use Test::Harness::Results;

# Flags used as return values from our methods.  Just for internal 
# clarification.
my $YES   = (1==1);
my $NO    = !$YES;

=head1 NAME

Test::Harness::Straps - detailed analysis of test results

=head1 SYNOPSIS

  use Test::Harness::Straps;

  my $strap = Test::Harness::Straps->new;

  # Various ways to interpret a test
  my $results = $strap->analyze($name, \@test_output);
  my $results = $strap->analyze_fh($name, $test_filehandle);
  my $results = $strap->analyze_file($test_file);

  # UNIMPLEMENTED
  my %total = $strap->total_results;

  # Altering the behavior of the strap  UNIMPLEMENTED
  my $verbose_output = $strap->dump_verbose();
  $strap->dump_verbose_fh($output_filehandle);


=head1 DESCRIPTION

B<THIS IS ALPHA SOFTWARE> in that the interface is subject to change
in incompatible ways.  It is otherwise stable.

Test::Harness is limited to printing out its results.  This makes
analysis of the test results difficult for anything but a human.  To
make it easier for programs to work with test results, we provide
Test::Harness::Straps.  Instead of printing the results, straps
provide them as raw data.  You can also configure how the tests are to
be run.

The interface is currently incomplete.  I<Please> contact the author
if you'd like a feature added or something change or just have
comments.

=head1 CONSTRUCTION

=head2 new()

  my $strap = Test::Harness::Straps->new;

Initialize a new strap.

=cut

sub new {
    my $class = shift;
    my $self  = bless {}, $class;

    $self->_init;

    return $self;
}

=for private $strap->_init

  $strap->_init;

Initialize the internal state of a strap to make it ready for parsing.

=cut

sub _init {
    my($self) = shift;

    $self->{_is_vms}   = ( $^O eq 'VMS' );
    $self->{_is_win32} = ( $^O =~ /^(MS)?Win32$/ );
    $self->{_is_macos} = ( $^O eq 'MacOS' );
}

=head1 ANALYSIS

=head2 $strap->analyze( $name, \@output_lines )

    my $results = $strap->analyze($name, \@test_output);

Analyzes the output of a single test, assigning it the given C<$name>
for use in the total report.  Returns the C<$results> of the test.
See L<Results>.

C<@test_output> should be the raw output from the test, including
newlines.

=cut

sub analyze {
    my($self, $name, $test_output) = @_;

    my $it = Test::Harness::Iterator->new($test_output);
    return $self->_analyze_iterator($name, $it);
}


sub _analyze_iterator {
    my($self, $name, $it) = @_;

    $self->_reset_file_state;
    $self->{file} = $name;

    my $results = Test::Harness::Results->new;

    # Set them up here so callbacks can have them.
    $self->{totals}{$name} = $results;
    while( defined(my $line = $it->next) ) {
        $self->_analyze_line($line, $results);
        last if $self->{saw_bailout};
    }

    $results->set_skip_all( $self->{skip_all} ) if defined $self->{skip_all};

    my $passed =
        (($results->max == 0) && defined $results->skip_all) ||
        ($results->max &&
         $results->seen &&
         $results->max == $results->seen &&
         $results->max == $results->ok);

    $results->set_passing( $passed ? 1 : 0 );

    return $results;
}


sub _analyze_line {
    my $self = shift;
    my $line = shift;
    my $results = shift;

    $self->{line}++;

    my $linetype;
    my $point = Test::Harness::Point->from_test_line( $line );
    if ( $point ) {
        $linetype = 'test';

        $results->inc_seen;
        $point->set_number( $self->{'next'} ) unless $point->number;

        # sometimes the 'not ' and the 'ok' are on different lines,
        # happens often on VMS if you do:
        #   print "not " unless $test;
        #   print "ok $num\n";
        if ( $self->{lone_not_line} && ($self->{lone_not_line} == $self->{line} - 1) ) {
            $point->set_ok( 0 );
        }

        if ( $self->{todo}{$point->number} ) {
            $point->set_directive_type( 'todo' );
        }

        if ( $point->is_todo ) {
            $results->inc_todo;
            $results->inc_bonus if $point->ok;
        }
        elsif ( $point->is_skip ) {
            $results->inc_skip;
        }

        $results->inc_ok if $point->pass;

        if ( ($point->number > 100_000) && ($point->number > ($self->{max}||100_000)) ) {
            if ( !$self->{too_many_tests}++ ) {
                warn "Enormous test number seen [test ", $point->number, "]\n";
                warn "Can't detailize, too big.\n";
            }
        }
        else {
            my $details = {
                ok          => $point->pass,
                actual_ok   => $point->ok,
                name        => _def_or_blank( $point->description ),
                type        => _def_or_blank( $point->directive_type ),
                reason      => _def_or_blank( $point->directive_reason ),
            };

            assert( defined( $details->{ok} ) && defined( $details->{actual_ok} ) );
            $results->set_details( $point->number, $details );
        }
    } # test point
    elsif ( $line =~ /^not\s+$/ ) {
        $linetype = 'other';
        # Sometimes the "not " and "ok" will be on separate lines on VMS.
        # We catch this and remember we saw it.
        $self->{lone_not_line} = $self->{line};
    }
    elsif ( $self->_is_header($line) ) {
        $linetype = 'header';

        $self->{saw_header}++;

        $results->inc_max( $self->{max} );
    }
    elsif ( $self->_is_bail_out($line, \$self->{bailout_reason}) ) {
        $linetype = 'bailout';
        $self->{saw_bailout} = 1;
    }
    elsif (my $diagnostics = $self->_is_diagnostic_line( $line )) {
        $linetype = 'other';
        # XXX We can throw this away, really.
        my $test = $results->details->[-1];
        $test->{diagnostics} ||=  '';
        $test->{diagnostics}  .= $diagnostics;
    }
    else {
        $linetype = 'other';
    }

    $self->callback->($self, $line, $linetype, $results) if $self->callback;

    $self->{'next'} = $point->number + 1 if $point;
} # _analyze_line


sub _is_diagnostic_line {
    my ($self, $line) = @_;
    return if index( $line, '# Looks like you failed' ) == 0;
    $line =~ s/^#\s//;
    return $line;
}

=for private $strap->analyze_fh( $name, $test_filehandle )

    my $results = $strap->analyze_fh($name, $test_filehandle);

Like C<analyze>, but it reads from the given filehandle.

=cut

sub analyze_fh {
    my($self, $name, $fh) = @_;

    my $it = Test::Harness::Iterator->new($fh);
    return $self->_analyze_iterator($name, $it);
}

=head2 $strap->analyze_file( $test_file )

    my $results = $strap->analyze_file($test_file);

Like C<analyze>, but it runs the given C<$test_file> and parses its
results.  It will also use that name for the total report.

=cut

sub analyze_file {
    my($self, $file) = @_;

    unless( -e $file ) {
        $self->{error} = "$file does not exist";
        return;
    }

    unless( -r $file ) {
        $self->{error} = "$file is not readable";
        return;
    }

    local $ENV{PERL5LIB} = $self->_INC2PERL5LIB;
    if ( $Test::Harness::Debug ) {
        local $^W=0; # ignore undef warnings
        print "# PERL5LIB=$ENV{PERL5LIB}\n";
    }

    # *sigh* this breaks under taint, but open -| is unportable.
    my $line = $self->_command_line($file);

    unless ( open(FILE, "$line|" )) {
        print "can't run $file. $!\n";
        return;
    }

    my $results = $self->analyze_fh($file, \*FILE);
    my $exit    = close FILE;

    $results->set_wait($?);
    if ( $? && $self->{_is_vms} ) {
        $results->set_exit($?);
    }
    else {
        $results->set_exit( _wait2exit($?) );
    }
    $results->set_passing(0) unless $? == 0;

    $self->_restore_PERL5LIB();

    return $results;
}


eval { require POSIX; &POSIX::WEXITSTATUS(0) };
if( $@ ) {
    *_wait2exit = sub { $_[0] >> 8 };
}
else {
    *_wait2exit = sub { POSIX::WEXITSTATUS($_[0]) }
}

=for private $strap->_command_line( $file )

Returns the full command line that will be run to test I<$file>.

=cut

sub _command_line {
    my $self = shift;
    my $file = shift;

    my $command =  $self->_command();
    my $switches = $self->_switches($file);

    $file = qq["$file"] if ($file =~ /\s/) && ($file !~ /^".*"$/);
    my $line = "$command $switches $file";

    return $line;
}


=for private $strap->_command()

Returns the command that runs the test.  Combine this with C<_switches()>
to build a command line.

Typically this is C<$^X>, but you can set C<$ENV{HARNESS_PERL}>
to use a different Perl than what you're running the harness under.
This might be to run a threaded Perl, for example.

You can also overload this method if you've built your own strap subclass,
such as a PHP interpreter for a PHP-based strap.

=cut

sub _command {
    my $self = shift;

    return $ENV{HARNESS_PERL}   if defined $ENV{HARNESS_PERL};
    #return qq["$^X"]            if $self->{_is_win32} && ($^X =~ /[^\w\.\/\\]/);
    return qq["$^X"]            if $^X =~ /\s/ and $^X !~ /^["']/;
    return $^X;
}


=for private $strap->_switches( $file )

Formats and returns the switches necessary to run the test.

=cut

sub _switches {
    my($self, $file) = @_;

    my @existing_switches = $self->_cleaned_switches( $Test::Harness::Switches, $ENV{HARNESS_PERL_SWITCHES} );
    my @derived_switches;

    local *TEST;
    open(TEST, $file) or print "can't open $file. $!\n";
    my $shebang = <TEST>;
    close(TEST) or print "can't close $file. $!\n";

    my $taint = ( $shebang =~ /^#!.*\bperl.*\s-\w*([Tt]+)/ );
    push( @derived_switches, "-$1" ) if $taint;

    # When taint mode is on, PERL5LIB is ignored.  So we need to put
    # all that on the command line as -Is.
    # MacPerl's putenv is broken, so it will not see PERL5LIB, tainted or not.
    if ( $taint || $self->{_is_macos} ) {
	my @inc = $self->_filtered_INC;
	push @derived_switches, map { "-I$_" } @inc;
    }

    # Quote the argument if there's any whitespace in it, or if
    # we're VMS, since VMS requires all parms quoted.  Also, don't quote
    # it if it's already quoted.
    for ( @derived_switches ) {
	$_ = qq["$_"] if ((/\s/ || $self->{_is_vms}) && !/^".*"$/ );
    }
    return join( " ", @existing_switches, @derived_switches );
}

=for private $strap->_cleaned_switches( @switches_from_user )

Returns only defined, non-blank, trimmed switches from the parms passed.

=cut

sub _cleaned_switches {
    my $self = shift;

    local $_;

    my @switches;
    for ( @_ ) {
	my $switch = $_;
	next unless defined $switch;
	$switch =~ s/^\s+//;
	$switch =~ s/\s+$//;
	push( @switches, $switch ) if $switch ne "";
    }

    return @switches;
}

=for private $strap->_INC2PERL5LIB

  local $ENV{PERL5LIB} = $self->_INC2PERL5LIB;

Takes the current value of C<@INC> and turns it into something suitable
for putting onto C<PERL5LIB>.

=cut

sub _INC2PERL5LIB {
    my($self) = shift;

    $self->{_old5lib} = $ENV{PERL5LIB};

    return join $Config{path_sep}, $self->_filtered_INC;
}

=for private $strap->_filtered_INC()

  my @filtered_inc = $self->_filtered_INC;

Shortens C<@INC> by removing redundant and unnecessary entries.
Necessary for OSes with limited command line lengths, like VMS.

=cut

sub _filtered_INC {
    my($self, @inc) = @_;
    @inc = @INC unless @inc;

    if( $self->{_is_vms} ) {
	# VMS has a 255-byte limit on the length of %ENV entries, so
	# toss the ones that involve perl_root, the install location
        @inc = grep !/perl_root/i, @inc;

    }
    elsif ( $self->{_is_win32} ) {
	# Lose any trailing backslashes in the Win32 paths
	s/[\\\/+]$// foreach @inc;
    }

    my %seen;
    $seen{$_}++ foreach $self->_default_inc();
    @inc = grep !$seen{$_}++, @inc;

    return @inc;
}


{ # Without caching, _default_inc() takes a huge amount of time
    my %cache;
    sub _default_inc {
        my $self = shift;
        my $perl = $self->_command;
        $cache{$perl} ||= [do {
            local $ENV{PERL5LIB};
            my @inc =`$perl -le "print join qq[\\n], \@INC"`;
            chomp @inc;
        }];
        return @{$cache{$perl}};
    }
}


=for private $strap->_restore_PERL5LIB()

  $self->_restore_PERL5LIB;

This restores the original value of the C<PERL5LIB> environment variable.
Necessary on VMS, otherwise a no-op.

=cut

sub _restore_PERL5LIB {
    my($self) = shift;

    return unless $self->{_is_vms};

    if (defined $self->{_old5lib}) {
        $ENV{PERL5LIB} = $self->{_old5lib};
    }
}

=head1 Parsing

Methods for identifying what sort of line you're looking at.

=for private _is_diagnostic

    my $is_diagnostic = $strap->_is_diagnostic($line, \$comment);

Checks if the given line is a comment.  If so, it will place it into
C<$comment> (sans #).

=cut

sub _is_diagnostic {
    my($self, $line, $comment) = @_;

    if( $line =~ /^\s*\#(.*)/ ) {
        $$comment = $1;
        return $YES;
    }
    else {
        return $NO;
    }
}

=for private _is_header

  my $is_header = $strap->_is_header($line);

Checks if the given line is a header (1..M) line.  If so, it places how
many tests there will be in C<< $strap->{max} >>, a list of which tests
are todo in C<< $strap->{todo} >> and if the whole test was skipped
C<< $strap->{skip_all} >> contains the reason.

=cut

# Regex for parsing a header.  Will be run with /x
my $Extra_Header_Re = <<'REGEX';
                       ^
                        (?: \s+ todo \s+ ([\d \t]+) )?      # optional todo set
                        (?: \s* \# \s* ([\w:]+\s?) (.*) )?     # optional skip with optional reason
REGEX

sub _is_header {
    my($self, $line) = @_;

    if( my($max, $extra) = $line =~ /^1\.\.(\d+)(.*)/ ) {
        $self->{max}  = $max;
        assert( $self->{max} >= 0,  'Max # of tests looks right' );

        if( defined $extra ) {
            my($todo, $skip, $reason) = $extra =~ /$Extra_Header_Re/xo;

            $self->{todo} = { map { $_ => 1 } split /\s+/, $todo } if $todo;

            if( $self->{max} == 0 ) {
                $reason = '' unless defined $skip and $skip =~ /^Skip/i;
            }

            $self->{skip_all} = $reason;
        }

        return $YES;
    }
    else {
        return $NO;
    }
}

=for private _is_bail_out

  my $is_bail_out = $strap->_is_bail_out($line, \$reason);

Checks if the line is a "Bail out!".  Places the reason for bailing
(if any) in $reason.

=cut

sub _is_bail_out {
    my($self, $line, $reason) = @_;

    if( $line =~ /^Bail out!\s*(.*)/i ) {
        $$reason = $1 if $1;
        return $YES;
    }
    else {
        return $NO;
    }
}

=for private _reset_file_state

  $strap->_reset_file_state;

Resets things like C<< $strap->{max} >> , C<< $strap->{skip_all} >>,
etc. so it's ready to parse the next file.

=cut

sub _reset_file_state {
    my($self) = shift;

    delete @{$self}{qw(max skip_all todo too_many_tests)};
    $self->{line}       = 0;
    $self->{saw_header} = 0;
    $self->{saw_bailout}= 0;
    $self->{lone_not_line} = 0;
    $self->{bailout_reason} = '';
    $self->{'next'}       = 1;
}

=head1 EXAMPLES

See F<examples/mini_harness.plx> for an example of use.

=head1 AUTHOR

Michael G Schwern C<< <schwern at pobox.com> >>, currently maintained by
Andy Lester C<< <andy at petdance.com> >>.

=head1 SEE ALSO

L<Test::Harness>

=cut

sub _def_or_blank {
    return $_[0] if defined $_[0];
    return "";
}

sub set_callback {
    my $self = shift;
    $self->{callback} = shift;
}

sub callback {
    my $self = shift;
    return $self->{callback};
}

1;
