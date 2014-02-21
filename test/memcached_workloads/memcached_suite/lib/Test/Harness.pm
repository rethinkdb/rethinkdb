# -*- Mode: cperl; cperl-indent-level: 4 -*-

package Test::Harness;

require 5.00405;
use Test::Harness::Straps;
use Test::Harness::Assert;
use Exporter;
use Benchmark;
use Config;
use strict;


use vars qw(
    $VERSION 
    @ISA @EXPORT @EXPORT_OK 
    $Verbose $Switches $Debug
    $verbose $switches $debug
    $Columns
    $Timer
    $ML $Last_ML_Print
    $Strap
    $has_time_hires
);

BEGIN {
    eval q{use Time::HiRes 'time'};
    $has_time_hires = !$@;
}

=head1 NAME

Test::Harness - Run Perl standard test scripts with statistics

=head1 VERSION

Version 2.64

=cut

$VERSION = '2.64';

# Backwards compatibility for exportable variable names.
*verbose  = *Verbose;
*switches = *Switches;
*debug    = *Debug;

$ENV{HARNESS_ACTIVE} = 1;
$ENV{HARNESS_VERSION} = $VERSION;

END {
    # For VMS.
    delete $ENV{HARNESS_ACTIVE};
    delete $ENV{HARNESS_VERSION};
}

my $Files_In_Dir = $ENV{HARNESS_FILELEAK_IN_DIR};

# Stolen from Params::Util
sub _CLASS {
    (defined $_[0] and ! ref $_[0] and $_[0] =~ m/^[^\W\d]\w*(?:::\w+)*$/s) ? $_[0] : undef;
}

# Strap Overloading
if ( $ENV{HARNESS_STRAPS_CLASS} ) {
    die 'Set HARNESS_STRAP_CLASS, singular, not HARNESS_STRAPS_CLASS';
}
my $HARNESS_STRAP_CLASS  = $ENV{HARNESS_STRAP_CLASS} || 'Test::Harness::Straps';
if ( $HARNESS_STRAP_CLASS =~ /\.pm$/ ) {
    # "Class" is actually a filename, that should return the
    # class name as its true return value.
    $HARNESS_STRAP_CLASS = require $HARNESS_STRAP_CLASS;
    if ( !_CLASS($HARNESS_STRAP_CLASS) ) {
        die "HARNESS_STRAP_CLASS '$HARNESS_STRAP_CLASS' is not a valid class name";
    }
}
else {
    # It is a class name within the current @INC
    if ( !_CLASS($HARNESS_STRAP_CLASS) ) {
        die "HARNESS_STRAP_CLASS '$HARNESS_STRAP_CLASS' is not a valid class name";
    }
    eval "require $HARNESS_STRAP_CLASS";
    die $@ if $@;
}
if ( !$HARNESS_STRAP_CLASS->isa('Test::Harness::Straps') ) {
    die "HARNESS_STRAP_CLASS '$HARNESS_STRAP_CLASS' must be a Test::Harness::Straps subclass";
}

$Strap = $HARNESS_STRAP_CLASS->new;

sub strap { return $Strap };

@ISA = ('Exporter');
@EXPORT    = qw(&runtests);
@EXPORT_OK = qw(&execute_tests $verbose $switches);

$Verbose  = $ENV{HARNESS_VERBOSE} || 0;
$Debug    = $ENV{HARNESS_DEBUG} || 0;
$Switches = '-w';
$Columns  = $ENV{HARNESS_COLUMNS} || $ENV{COLUMNS} || 80;
$Columns--;             # Some shells have trouble with a full line of text.
$Timer    = $ENV{HARNESS_TIMER} || 0;

=head1 SYNOPSIS

  use Test::Harness;

  runtests(@test_files);

=head1 DESCRIPTION

B<STOP!> If all you want to do is write a test script, consider
using Test::Simple.  Test::Harness is the module that reads the
output from Test::Simple, Test::More and other modules based on
Test::Builder.  You don't need to know about Test::Harness to use
those modules.

Test::Harness runs tests and expects output from the test in a
certain format.  That format is called TAP, the Test Anything
Protocol.  It is defined in L<Test::Harness::TAP>.

C<Test::Harness::runtests(@tests)> runs all the testscripts named
as arguments and checks standard output for the expected strings
in TAP format.

The F<prove> utility is a thin wrapper around Test::Harness.

=head2 Taint mode

Test::Harness will honor the C<-T> or C<-t> in the #! line on your
test files.  So if you begin a test with:

    #!perl -T

the test will be run with taint mode on.

=head2 Configuration variables.

These variables can be used to configure the behavior of
Test::Harness.  They are exported on request.

=over 4

=item C<$Test::Harness::Verbose>

The package variable C<$Test::Harness::Verbose> is exportable and can be
used to let C<runtests()> display the standard output of the script
without altering the behavior otherwise.  The F<prove> utility's C<-v>
flag will set this.

=item C<$Test::Harness::switches>

The package variable C<$Test::Harness::switches> is exportable and can be
used to set perl command line options used for running the test
script(s). The default value is C<-w>. It overrides C<HARNESS_PERL_SWITCHES>.

=item C<$Test::Harness::Timer>

If set to true, and C<Time::HiRes> is available, print elapsed seconds
after each test file.

=back


=head2 Failure

When tests fail, analyze the summary report:

  t/base..............ok
  t/nonumbers.........ok
  t/ok................ok
  t/test-harness......ok
  t/waterloo..........dubious
          Test returned status 3 (wstat 768, 0x300)
  DIED. FAILED tests 1, 3, 5, 7, 9, 11, 13, 15, 17, 19
          Failed 10/20 tests, 50.00% okay
  Failed Test  Stat Wstat Total Fail  List of Failed
  ---------------------------------------------------------------
  t/waterloo.t    3   768    20   10  1 3 5 7 9 11 13 15 17 19
  Failed 1/5 test scripts, 80.00% okay. 10/44 subtests failed, 77.27% okay.

Everything passed but F<t/waterloo.t>.  It failed 10 of 20 tests and
exited with non-zero status indicating something dubious happened.

The columns in the summary report mean:

=over 4

=item B<Failed Test>

The test file which failed.

=item B<Stat>

If the test exited with non-zero, this is its exit status.

=item B<Wstat>

The wait status of the test.

=item B<Total>

Total number of tests expected to run.

=item B<Fail>

Number which failed, either from "not ok" or because they never ran.

=item B<List of Failed>

A list of the tests which failed.  Successive failures may be
abbreviated (ie. 15-20 to indicate that tests 15, 16, 17, 18, 19 and
20 failed).

=back


=head1 FUNCTIONS

The following functions are available.

=head2 runtests( @test_files )

This runs all the given I<@test_files> and divines whether they passed
or failed based on their output to STDOUT (details above).  It prints
out each individual test which failed along with a summary report and
a how long it all took.

It returns true if everything was ok.  Otherwise it will C<die()> with
one of the messages in the DIAGNOSTICS section.

=cut

sub runtests {
    my(@tests) = @_;

    local ($\, $,);

    my ($tot, $failedtests,$todo_passed) = execute_tests(tests => \@tests);
    print get_results($tot, $failedtests,$todo_passed);

    my $ok = _all_ok($tot);

    assert(($ok xor keys %$failedtests), 
           q{ok status jives with $failedtests});

    if (! $ok) {
        die("Failed $tot->{bad}/$tot->{tests} test programs. " .
            "@{[$tot->{max} - $tot->{ok}]}/$tot->{max} subtests failed.\n");
    }

    return $ok;
}

# my $ok = _all_ok(\%tot);
# Tells you if this test run is overall successful or not.

sub _all_ok {
    my($tot) = shift;

    return $tot->{bad} == 0 && ($tot->{max} || $tot->{skipped}) ? 1 : 0;
}

# Returns all the files in a directory.  This is shorthand for backwards
# compatibility on systems where C<glob()> doesn't work right.

sub _globdir {
    local *DIRH;

    opendir DIRH, shift;
    my @f = readdir DIRH;
    closedir DIRH;

    return @f;
}

=head2 execute_tests( tests => \@test_files, out => \*FH )

Runs all the given C<@test_files> (just like C<runtests()>) but
doesn't generate the final report.  During testing, progress
information will be written to the currently selected output
filehandle (usually C<STDOUT>), or to the filehandle given by the
C<out> parameter.  The I<out> is optional.

Returns a list of two values, C<$total> and C<$failed>, describing the
results.  C<$total> is a hash ref summary of all the tests run.  Its
keys and values are this:

    bonus           Number of individual todo tests unexpectedly passed
    max             Number of individual tests ran
    ok              Number of individual tests passed
    sub_skipped     Number of individual tests skipped
    todo            Number of individual todo tests

    files           Number of test files ran
    good            Number of test files passed
    bad             Number of test files failed
    tests           Number of test files originally given
    skipped         Number of test files skipped

If C<< $total->{bad} == 0 >> and C<< $total->{max} > 0 >>, you've
got a successful test.

C<$failed> is a hash ref of all the test scripts that failed.  Each key
is the name of a test script, each value is another hash representing
how that script failed.  Its keys are these:

    name        Name of the test which failed
    estat       Script's exit value
    wstat       Script's wait status
    max         Number of individual tests
    failed      Number which failed
    canon       List of tests which failed (as string).

C<$failed> should be empty if everything passed.

=cut

sub execute_tests {
    my %args = @_;
    my @tests = @{$args{tests}};
    my $out = $args{out} || select();

    # We allow filehandles that are symbolic refs
    no strict 'refs';
    _autoflush($out);
    _autoflush(\*STDERR);

    my %failedtests;
    my %todo_passed;

    # Test-wide totals.
    my(%tot) = (
                bonus    => 0,
                max      => 0,
                ok       => 0,
                files    => 0,
                bad      => 0,
                good     => 0,
                tests    => scalar @tests,
                sub_skipped  => 0,
                todo     => 0,
                skipped  => 0,
                bench    => 0,
               );

    my @dir_files;
    @dir_files = _globdir $Files_In_Dir if defined $Files_In_Dir;
    my $run_start_time = new Benchmark;

    my $width = _leader_width(@tests);
    foreach my $tfile (@tests) {
        $Last_ML_Print = 0;  # so each test prints at least once
        my($leader, $ml) = _mk_leader($tfile, $width);
        local $ML = $ml;

        print $out $leader;

        $tot{files}++;

        $Strap->{_seen_header} = 0;
        if ( $Test::Harness::Debug ) {
            print $out "# Running: ", $Strap->_command_line($tfile), "\n";
        }
        my $test_start_time = $Timer ? time : 0;
        my $results = $Strap->analyze_file($tfile) or
          do { warn $Strap->{error}, "\n";  next };
        my $elapsed;
        if ( $Timer ) {
            $elapsed = time - $test_start_time;
            if ( $has_time_hires ) {
                $elapsed = sprintf( " %8d ms", $elapsed*1000 );
            }
            else {
                $elapsed = sprintf( " %8s s", $elapsed ? $elapsed : "<1" );
            }
        }
        else {
            $elapsed = "";
        }

        # state of the current test.
        my @failed = grep { !$results->details->[$_-1]{ok} }
                     1..@{$results->details};
        my @todo_pass = grep { $results->details->[$_-1]{actual_ok} &&
                               $results->details->[$_-1]{type} eq 'todo' }
                        1..@{$results->details};

        my %test = (
            ok          => $results->ok,
            'next'      => $Strap->{'next'},
            max         => $results->max,
            failed      => \@failed,
            todo_pass   => \@todo_pass,
            todo        => $results->todo,
            bonus       => $results->bonus,
            skipped     => $results->skip,
            skip_reason => $results->skip_reason,
            skip_all    => $Strap->{skip_all},
            ml          => $ml,
        );

        $tot{bonus}       += $results->bonus;
        $tot{max}         += $results->max;
        $tot{ok}          += $results->ok;
        $tot{todo}        += $results->todo;
        $tot{sub_skipped} += $results->skip;

        my $estatus = $results->exit;
        my $wstatus = $results->wait;

        if ( $results->passing ) {
            # XXX Combine these first two
            if ($test{max} and $test{skipped} + $test{bonus}) {
                my @msg;
                push(@msg, "$test{skipped}/$test{max} skipped: $test{skip_reason}")
                    if $test{skipped};
                if ($test{bonus}) {
                    my ($txt, $canon) = _canondetail($test{todo},0,'TODO passed',
                                                    @{$test{todo_pass}});
                    $todo_passed{$tfile} = {
                        canon   => $canon,
                        max     => $test{todo},
                        failed  => $test{bonus},
                        name    => $tfile,
                        estat   => '',
                        wstat   => '',
                    };

                    push(@msg, "$test{bonus}/$test{max} unexpectedly succeeded\n$txt");
                }
                print $out "$test{ml}ok$elapsed\n        ".join(', ', @msg)."\n";
            }
            elsif ( $test{max} ) {
                print $out "$test{ml}ok$elapsed\n";
            }
            elsif ( defined $test{skip_all} and length $test{skip_all} ) {
                print $out "skipped\n        all skipped: $test{skip_all}\n";
                $tot{skipped}++;
            }
            else {
                print $out "skipped\n        all skipped: no reason given\n";
                $tot{skipped}++;
            }
            $tot{good}++;
        }
        else {
            # List unrun tests as failures.
            if ($test{'next'} <= $test{max}) {
                push @{$test{failed}}, $test{'next'}..$test{max};
            }
            # List overruns as failures.
            else {
                my $details = $results->details;
                foreach my $overrun ($test{max}+1..@$details) {
                    next unless ref $details->[$overrun-1];
                    push @{$test{failed}}, $overrun
                }
            }

            if ($wstatus) {
                $failedtests{$tfile} = _dubious_return(\%test, \%tot, 
                                                       $estatus, $wstatus);
                $failedtests{$tfile}{name} = $tfile;
            }
            elsif ( $results->seen ) {
                if (@{$test{failed}} and $test{max}) {
                    my ($txt, $canon) = _canondetail($test{max},$test{skipped},'Failed',
                                                    @{$test{failed}});
                    print $out "$test{ml}$txt";
                    $failedtests{$tfile} = { canon   => $canon,
                                             max     => $test{max},
                                             failed  => scalar @{$test{failed}},
                                             name    => $tfile, 
                                             estat   => '',
                                             wstat   => '',
                                           };
                }
                else {
                    print $out "Don't know which tests failed: got $test{ok} ok, ".
                          "expected $test{max}\n";
                    $failedtests{$tfile} = { canon   => '??',
                                             max     => $test{max},
                                             failed  => '??',
                                             name    => $tfile, 
                                             estat   => '', 
                                             wstat   => '',
                                           };
                }
                $tot{bad}++;
            }
            else {
                print $out "FAILED before any test output arrived\n";
                $tot{bad}++;
                $failedtests{$tfile} = { canon       => '??',
                                         max         => '??',
                                         failed      => '??',
                                         name        => $tfile,
                                         estat       => '', 
                                         wstat       => '',
                                       };
            }
        }

        if (defined $Files_In_Dir) {
            my @new_dir_files = _globdir $Files_In_Dir;
            if (@new_dir_files != @dir_files) {
                my %f;
                @f{@new_dir_files} = (1) x @new_dir_files;
                delete @f{@dir_files};
                my @f = sort keys %f;
                print $out "LEAKED FILES: @f\n";
                @dir_files = @new_dir_files;
            }
        }
    } # foreach test
    $tot{bench} = timediff(new Benchmark, $run_start_time);

    $Strap->_restore_PERL5LIB;

    return(\%tot, \%failedtests, \%todo_passed);
}

# Turns on autoflush for the handle passed
sub _autoflush {
    my $flushy_fh = shift;
    my $old_fh = select $flushy_fh;
    $| = 1;
    select $old_fh;
}

=for private _mk_leader

    my($leader, $ml) = _mk_leader($test_file, $width);

Generates the 't/foo........' leader for the given C<$test_file> as well
as a similar version which will overwrite the current line (by use of
\r and such).  C<$ml> may be empty if Test::Harness doesn't think you're
on TTY.

The C<$width> is the width of the "yada/blah.." string.

=cut

sub _mk_leader {
    my($te, $width) = @_;
    chomp($te);
    $te =~ s/\.\w+$/./;

    if ($^O eq 'VMS') {
        $te =~ s/^.*\.t\./\[.t./s;
    }
    my $leader = "$te" . '.' x ($width - length($te));
    my $ml = "";

    if ( -t STDOUT and not $ENV{HARNESS_NOTTY} and not $Verbose ) {
        $ml = "\r" . (' ' x 77) . "\r$leader"
    }

    return($leader, $ml);
}

=for private _leader_width

  my($width) = _leader_width(@test_files);

Calculates how wide the leader should be based on the length of the
longest test name.

=cut

sub _leader_width {
    my $maxlen = 0;
    my $maxsuflen = 0;
    foreach (@_) {
        my $suf    = /\.(\w+)$/ ? $1 : '';
        my $len    = length;
        my $suflen = length $suf;
        $maxlen    = $len    if $len    > $maxlen;
        $maxsuflen = $suflen if $suflen > $maxsuflen;
    }
    # + 3 : we want three dots between the test name and the "ok"
    return $maxlen + 3 - $maxsuflen;
}

sub get_results {
    my $tot = shift;
    my $failedtests = shift;
    my $todo_passed = shift;

    my $out = '';

    my $bonusmsg = _bonusmsg($tot);

    if (_all_ok($tot)) {
        $out .= "All tests successful$bonusmsg.\n";
        if ($tot->{bonus}) {
            my($fmt_top, $fmt) = _create_fmts("Passed TODO",$todo_passed);
            # Now write to formats
            $out .= swrite( $fmt_top );
            for my $script (sort keys %{$todo_passed||{}}) {
                my $Curtest = $todo_passed->{$script};
                $out .= swrite( $fmt, @{ $Curtest }{qw(name estat wstat max failed canon)} );
            }
        }
    }
    elsif (!$tot->{tests}){
        die "FAILED--no tests were run for some reason.\n";
    }
    elsif (!$tot->{max}) {
        my $blurb = $tot->{tests}==1 ? "script" : "scripts";
        die "FAILED--$tot->{tests} test $blurb could be run, ".
            "alas--no output ever seen\n";
    }
    else {
        my $subresults = sprintf( " %d/%d subtests failed.",
                              $tot->{max} - $tot->{ok}, $tot->{max} );

        my($fmt_top, $fmt1, $fmt2) = _create_fmts("Failed Test",$failedtests);

        # Now write to formats
        $out .= swrite( $fmt_top );
        for my $script (sort keys %$failedtests) {
            my $Curtest = $failedtests->{$script};
            $out .= swrite( $fmt1, @{ $Curtest }{qw(name estat wstat max failed canon)} );
            $out .= swrite( $fmt2, $Curtest->{canon} );
        }
        if ($tot->{bad}) {
            $bonusmsg =~ s/^,\s*//;
            $out .= "$bonusmsg.\n" if $bonusmsg;
            $out .= "Failed $tot->{bad}/$tot->{tests} test scripts.$subresults\n";
        }
    }

    $out .= sprintf("Files=%d, Tests=%d, %s\n",
           $tot->{files}, $tot->{max}, timestr($tot->{bench}, 'nop'));
    return $out;
}

sub swrite {
    my $format = shift;
    $^A = '';
    formline($format,@_);
    my $out = $^A;
    $^A = '';
    return $out;
}


my %Handlers = (
    header  => \&header_handler,
    test    => \&test_handler,
    bailout => \&bailout_handler,
);

$Strap->set_callback(\&strap_callback);
sub strap_callback {
    my($self, $line, $type, $totals) = @_;
    print $line if $Verbose;

    my $meth = $Handlers{$type};
    $meth->($self, $line, $type, $totals) if $meth;
};


sub header_handler {
    my($self, $line, $type, $totals) = @_;

    warn "Test header seen more than once!\n" if $self->{_seen_header};

    $self->{_seen_header}++;

    warn "1..M can only appear at the beginning or end of tests\n"
      if $totals->seen && ($totals->max < $totals->seen);
};

sub test_handler {
    my($self, $line, $type, $totals) = @_;

    my $curr = $totals->seen;
    my $next = $self->{'next'};
    my $max  = $totals->max;
    my $detail = $totals->details->[-1];

    if( $detail->{ok} ) {
        _print_ml_less("ok $curr/$max");

        if( $detail->{type} eq 'skip' ) {
            $totals->set_skip_reason( $detail->{reason} )
              unless defined $totals->skip_reason;
            $totals->set_skip_reason( 'various reasons' )
              if $totals->skip_reason ne $detail->{reason};
        }
    }
    else {
        _print_ml("NOK $curr/$max");
    }

    if( $curr > $next ) {
        print "Test output counter mismatch [test $curr]\n";
    }
    elsif( $curr < $next ) {
        print "Confused test output: test $curr answered after ".
              "test ", $next - 1, "\n";
    }

};

sub bailout_handler {
    my($self, $line, $type, $totals) = @_;

    die "FAILED--Further testing stopped" .
      ($self->{bailout_reason} ? ": $self->{bailout_reason}\n" : ".\n");
};


sub _print_ml {
    print join '', $ML, @_ if $ML;
}


# Print updates only once per second.
sub _print_ml_less {
    my $now = CORE::time;
    if ( $Last_ML_Print != $now ) {
        _print_ml(@_);
        $Last_ML_Print = $now;
    }
}

sub _bonusmsg {
    my($tot) = @_;

    my $bonusmsg = '';
    $bonusmsg = (" ($tot->{bonus} subtest".($tot->{bonus} > 1 ? 's' : '').
               " UNEXPECTEDLY SUCCEEDED)")
        if $tot->{bonus};

    if ($tot->{skipped}) {
        $bonusmsg .= ", $tot->{skipped} test"
                     . ($tot->{skipped} != 1 ? 's' : '');
        if ($tot->{sub_skipped}) {
            $bonusmsg .= " and $tot->{sub_skipped} subtest"
                         . ($tot->{sub_skipped} != 1 ? 's' : '');
        }
        $bonusmsg .= ' skipped';
    }
    elsif ($tot->{sub_skipped}) {
        $bonusmsg .= ", $tot->{sub_skipped} subtest"
                     . ($tot->{sub_skipped} != 1 ? 's' : '')
                     . " skipped";
    }
    return $bonusmsg;
}

# Test program go boom.
sub _dubious_return {
    my($test, $tot, $estatus, $wstatus) = @_;

    my $failed = '??';
    my $canon  = '??';

    printf "$test->{ml}dubious\n\tTest returned status $estatus ".
           "(wstat %d, 0x%x)\n",
           $wstatus,$wstatus;
    print "\t\t(VMS status is $estatus)\n" if $^O eq 'VMS';

    $tot->{bad}++;

    if ($test->{max}) {
        if ($test->{'next'} == $test->{max} + 1 and not @{$test->{failed}}) {
            print "\tafter all the subtests completed successfully\n";
            $failed = 0;        # But we do not set $canon!
        }
        else {
            push @{$test->{failed}}, $test->{'next'}..$test->{max};
            $failed = @{$test->{failed}};
            (my $txt, $canon) = _canondetail($test->{max},$test->{skipped},'Failed',@{$test->{failed}});
            print "DIED. ",$txt;
        }
    }

    return { canon => $canon,  max => $test->{max} || '??',
             failed => $failed, 
             estat => $estatus, wstat => $wstatus,
           };
}


sub _create_fmts {
    my $failed_str = shift;
    my $failedtests = shift;

    my ($type) = split /\s/,$failed_str;
    my $short = substr($type,0,4);
    my $total = $short eq 'Pass' ? 'TODOs' : 'Total';
    my $middle_str = " Stat Wstat $total $short  ";
    my $list_str = "List of $type";

    # Figure out our longest name string for formatting purposes.
    my $max_namelen = length($failed_str);
    foreach my $script (keys %$failedtests) {
        my $namelen = length $failedtests->{$script}->{name};
        $max_namelen = $namelen if $namelen > $max_namelen;
    }

    my $list_len = $Columns - length($middle_str) - $max_namelen;
    if ($list_len < length($list_str)) {
        $list_len = length($list_str);
        $max_namelen = $Columns - length($middle_str) - $list_len;
        if ($max_namelen < length($failed_str)) {
            $max_namelen = length($failed_str);
            $Columns = $max_namelen + length($middle_str) + $list_len;
        }
    }

    my $fmt_top =   sprintf("%-${max_namelen}s", $failed_str)
                  . $middle_str
                  . $list_str . "\n"
                  . "-" x $Columns
                  . "\n";

    my $fmt1 =  "@" . "<" x ($max_namelen - 1)
              . "  @>> @>>>> @>>>> @>>>  "
              . "^" . "<" x ($list_len - 1) . "\n";
    my $fmt2 =  "~~" . " " x ($Columns - $list_len - 2) . "^"
              . "<" x ($list_len - 1) . "\n";

    return($fmt_top, $fmt1, $fmt2);
}

sub _canondetail {
    my $max = shift;
    my $skipped = shift;
    my $type = shift;
    my @detail = @_;
    my %seen;
    @detail = sort {$a <=> $b} grep !$seen{$_}++, @detail;
    my $detail = @detail;
    my @result = ();
    my @canon = ();
    my $min;
    my $last = $min = shift @detail;
    my $canon;
    my $uc_type = uc($type);
    if (@detail) {
        for (@detail, $detail[-1]) { # don't forget the last one
            if ($_ > $last+1 || $_ == $last) {
                push @canon, ($min == $last) ? $last : "$min-$last";
                $min = $_;
            }
            $last = $_;
        }
        local $" = ", ";
        push @result, "$uc_type tests @canon\n";
        $canon = join ' ', @canon;
    }
    else {
        push @result, "$uc_type test $last\n";
        $canon = $last;
    }

    return (join("", @result), $canon)
        if $type=~/todo/i;
    push @result, "\t$type $detail/$max tests, ";
    if ($max) {
	push @result, sprintf("%.2f",100*(1-$detail/$max)), "% okay";
    }
    else {
	push @result, "?% okay";
    }
    my $ender = 's' x ($skipped > 1);
    if ($skipped) {
        my $good = $max - $detail - $skipped;
	my $skipmsg = " (less $skipped skipped test$ender: $good okay, ";
	if ($max) {
	    my $goodper = sprintf("%.2f",100*($good/$max));
	    $skipmsg .= "$goodper%)";
        }
        else {
	    $skipmsg .= "?%)";
	}
	push @result, $skipmsg;
    }
    push @result, "\n";
    my $txt = join "", @result;
    return ($txt, $canon);
}

1;
__END__


=head1 EXPORT

C<&runtests> is exported by Test::Harness by default.

C<&execute_tests>, C<$verbose>, C<$switches> and C<$debug> are
exported upon request.

=head1 DIAGNOSTICS

=over 4

=item C<All tests successful.\nFiles=%d,  Tests=%d, %s>

If all tests are successful some statistics about the performance are
printed.

=item C<FAILED tests %s\n\tFailed %d/%d tests, %.2f%% okay.>

For any single script that has failing subtests statistics like the
above are printed.

=item C<Test returned status %d (wstat %d)>

Scripts that return a non-zero exit status, both C<$? E<gt>E<gt> 8>
and C<$?> are printed in a message similar to the above.

=item C<Failed 1 test, %.2f%% okay. %s>

=item C<Failed %d/%d tests, %.2f%% okay. %s>

If not all tests were successful, the script dies with one of the
above messages.

=item C<FAILED--Further testing stopped: %s>

If a single subtest decides that further testing will not make sense,
the script dies with this message.

=back

=head1 ENVIRONMENT VARIABLES THAT TEST::HARNESS SETS

Test::Harness sets these before executing the individual tests.

=over 4

=item C<HARNESS_ACTIVE>

This is set to a true value.  It allows the tests to determine if they
are being executed through the harness or by any other means.

=item C<HARNESS_VERSION>

This is the version of Test::Harness.

=back

=head1 ENVIRONMENT VARIABLES THAT AFFECT TEST::HARNESS

=over 4

=item C<HARNESS_COLUMNS>

This value will be used for the width of the terminal. If it is not
set then it will default to C<COLUMNS>. If this is not set, it will
default to 80. Note that users of Bourne-sh based shells will need to
C<export COLUMNS> for this module to use that variable.

=item C<HARNESS_COMPILE_TEST>

When true it will make harness attempt to compile the test using
C<perlcc> before running it.

B<NOTE> This currently only works when sitting in the perl source
directory!

=item C<HARNESS_DEBUG>

If true, Test::Harness will print debugging information about itself as
it runs the tests.  This is different from C<HARNESS_VERBOSE>, which prints
the output from the test being run.  Setting C<$Test::Harness::Debug> will
override this, or you can use the C<-d> switch in the F<prove> utility.

=item C<HARNESS_FILELEAK_IN_DIR>

When set to the name of a directory, harness will check after each
test whether new files appeared in that directory, and report them as

  LEAKED FILES: scr.tmp 0 my.db

If relative, directory name is with respect to the current directory at
the moment runtests() was called.  Putting absolute path into 
C<HARNESS_FILELEAK_IN_DIR> may give more predictable results.

=item C<HARNESS_NOTTY>

When set to a true value, forces it to behave as though STDOUT were
not a console.  You may need to set this if you don't want harness to
output more frequent progress messages using carriage returns.  Some
consoles may not handle carriage returns properly (which results in a
somewhat messy output).

=item C<HARNESS_PERL>

Usually your tests will be run by C<$^X>, the currently-executing Perl.
However, you may want to have it run by a different executable, such as
a threading perl, or a different version.

If you're using the F<prove> utility, you can use the C<--perl> switch.

=item C<HARNESS_PERL_SWITCHES>

Its value will be prepended to the switches used to invoke perl on
each test.  For example, setting C<HARNESS_PERL_SWITCHES> to C<-W> will
run all tests with all warnings enabled.

=item C<HARNESS_TIMER>

Setting this to true will make the harness display the number of
milliseconds each test took.  You can also use F<prove>'s C<--timer>
switch.

=item C<HARNESS_VERBOSE>

If true, Test::Harness will output the verbose results of running
its tests.  Setting C<$Test::Harness::verbose> will override this,
or you can use the C<-v> switch in the F<prove> utility.

If true, Test::Harness will output the verbose results of running
its tests.  Setting C<$Test::Harness::verbose> will override this,
or you can use the C<-v> switch in the F<prove> utility.

=item C<HARNESS_STRAP_CLASS>

Defines the Test::Harness::Straps subclass to use.  The value may either
be a filename or a class name.

If HARNESS_STRAP_CLASS is a class name, the class must be in C<@INC>
like any other class.

If HARNESS_STRAP_CLASS is a filename, the .pm file must return the name
of the class, instead of the canonical "1".

=back

=head1 EXAMPLE

Here's how Test::Harness tests itself

  $ cd ~/src/devel/Test-Harness
  $ perl -Mblib -e 'use Test::Harness qw(&runtests $verbose);
    $verbose=0; runtests @ARGV;' t/*.t
  Using /home/schwern/src/devel/Test-Harness/blib
  t/base..............ok
  t/nonumbers.........ok
  t/ok................ok
  t/test-harness......ok
  All tests successful.
  Files=4, Tests=24, 2 wallclock secs ( 0.61 cusr + 0.41 csys = 1.02 CPU)

=head1 SEE ALSO

The included F<prove> utility for running test scripts from the command line,
L<Test> and L<Test::Simple> for writing test scripts, L<Benchmark> for
the underlying timing routines, and L<Devel::Cover> for test coverage
analysis.

=head1 TODO

Provide a way of running tests quietly (ie. no printing) for automated
validation of tests.  This will probably take the form of a version
of runtests() which rather than printing its output returns raw data
on the state of the tests.  (Partially done in Test::Harness::Straps)

Document the format.

Fix HARNESS_COMPILE_TEST without breaking its core usage.

Figure a way to report test names in the failure summary.

Rework the test summary so long test names are not truncated as badly.
(Partially done with new skip test styles)

Add option for coverage analysis.

Trap STDERR.

Implement Straps total_results()

Remember exit code

Completely redo the print summary code.

Straps->analyze_file() not taint clean, don't know if it can be

Fix that damned VMS nit.

Add a test for verbose.

Change internal list of test results to a hash.

Fix stats display when there's an overrun.

Fix so perls with spaces in the filename work.

Keeping whittling away at _run_all_tests()

Clean up how the summary is printed.  Get rid of those damned formats.

=head1 BUGS

Please report any bugs or feature requests to
C<bug-test-harness at rt.cpan.org>, or through the web interface at
L<http://rt.cpan.org/NoAuth/ReportBug.html?Queue=Test-Harness>.
I will be notified, and then you'll automatically be notified of progress on
your bug as I make changes.

=head1 SUPPORT

You can find documentation for this module with the F<perldoc> command.

    perldoc Test::Harness

You can get docs for F<prove> with

    prove --man

You can also look for information at:

=over 4

=item * AnnoCPAN: Annotated CPAN documentation

L<http://annocpan.org/dist/Test-Harness>

=item * CPAN Ratings

L<http://cpanratings.perl.org/d/Test-Harness>

=item * RT: CPAN's request tracker

L<http://rt.cpan.org/NoAuth/Bugs.html?Dist=Test-Harness>

=item * Search CPAN

L<http://search.cpan.org/dist/Test-Harness>

=back

=head1 SOURCE CODE

The source code repository for Test::Harness is at
L<http://svn.perl.org/modules/Test-Harness>.

=head1 AUTHORS

Either Tim Bunce or Andreas Koenig, we don't know. What we know for
sure is, that it was inspired by Larry Wall's F<TEST> script that came
with perl distributions for ages. Numerous anonymous contributors
exist.  Andreas Koenig held the torch for many years, and then
Michael G Schwern.

Current maintainer is Andy Lester C<< <andy at petdance.com> >>.

=head1 COPYRIGHT

Copyright 2002-2006
by Michael G Schwern C<< <schwern at pobox.com> >>,
Andy Lester C<< <andy at petdance.com> >>.

This program is free software; you can redistribute it and/or 
modify it under the same terms as Perl itself.

See L<http://www.perl.com/perl/misc/Artistic.html>.

=cut
