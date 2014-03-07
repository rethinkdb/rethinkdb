#!/usr/local/bin/perl
#  ***********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002-2008, International Business Machines Corporation
#  * and others. All Rights Reserved.
#  ***********************************************************************

use strict;

#use Dataset;
use Format;
use Output;

my $VERBOSE = 0;
my $DEBUG   = 1;
my $start_l = ""; #formatting help
my $end_l   = "";
my @testArgs; # different kinds of tests we want to do
my $datadir = "data";
my $extraArgs; # stuff that always gets passed to the test program


my $iterCount = 0;
my $NUMPASSES = 4;
my $TIME = 2;
my $ITERATIONS;   #Added by Doug
my $DATADIR;

sub setupOptions {
  my %options = %{shift @_};

  if($options{"time"}) {
    $TIME = $options{"time"};
  }

  if($options{"passes"}) {
    $NUMPASSES = $options{"passes"};
  }
	
  if($options{"dataDir"}) {
    $DATADIR = $options{"dataDir"};
  }
  
  # Added by Doug
  if ($options{"iterations"}) {
  	$ITERATIONS = $options{"iterations"};
  }
}

sub runTests {
  my $options = shift;
  my @programs;
  my $tests = shift;
  my %datafiles;
  if($#_ >= 0) { # maybe no files/locales
    my $datafiles = shift;
    if($datafiles) {
      %datafiles = %{$datafiles};
    }
  }
  setupOutput($options);
  setupOptions($options);

  my($locale, $iter, $data, $program, $args, $variable);
#
#  Outer loop runs through the locales to test
#
  if (%datafiles) {
    foreach $locale (sort keys %datafiles ) {
      foreach $data (@{ $datafiles{$locale} }) {
	closeTable;
	my $locdata = "";
	if(!($locale eq "")) {
	  $locdata = "<b>Locale:</b> $locale<br>";
	}
	$locdata .= "<b>Datafile:</b> $data<br>";
	startTest($locdata);

	if($DATADIR) {
	  compareLoop ($tests, $locale, $DATADIR."/".$data);
	} else {
	  compareLoop ($tests, $locale, $data);
	}
      }
    }
  } else {
    compareLoop($tests);
  }
  closeOutput();
}

sub compareLoop {
  my $tests = shift;
  #my @tests = @{$tests};
  my %tests = %{$tests};
  my $locale = shift;
  my $datafile = shift;
  my $locAndData = "";
  if($locale) {
    $locAndData .= " -L $locale";
  }
  if($datafile) {
    $locAndData .= " -f $datafile";
  }
	  
  my $args;
  my ($i, $j, $aref);
  foreach $i ( sort keys %tests ) {
    debug("Test: $i\n");
    $aref = $tests{$i};
    my @timedata;
    my @iterPerPass;
    my @noopers;
    my @noevents;

    my $program;
    my @argsAndTest;
    for $j ( 0 .. $#{$aref} ) {
    # first we calibrate. Use time from somewhere
    # first test is used for calibration
      ($program, @argsAndTest) = split(/\ /, @{ $tests{$i} }[$j]);
      #Modified by Doug
      my $commandLine;
      if ($ITERATIONS) {
      	$commandLine = "$program -i $ITERATIONS -p $NUMPASSES $locAndData @argsAndTest";
      } else {
      	$commandLine = "$program -t $TIME -p $NUMPASSES $locAndData @argsAndTest";
    	}
      #my $commandLine = "$program -i 5 -p $NUMPASSES $locAndData @argsAndTest";
      my @res = measure1($commandLine);
      store("$i, $program @argsAndTest", @res);

      push(@iterPerPass, shift(@res));
      push(@noopers, shift(@res));
      my @data = @{ shift(@res) };
      if($#res >= 0) {
	push(@noevents, shift(@res));
      }
    

      shift(@data) if (@data > 1); # discard first run

      #debug("data is @data\n");
      my $ds = Dataset->new(@data);

      push(@timedata, $ds);
    }

    outputRow($i, \@iterPerPass, \@noopers, \@timedata, \@noevents);
  }

}

#---------------------------------------------------------------------
# Measure a given test method with a give test pattern using the
# global run parameters.
#
# @param the method to run
# @param the pattern defining characters to test
# @param if >0 then the number of iterations per pass.  If <0 then
#        (negative of) the number of seconds per pass.
#
# @return array of:
#         [0] iterations per pass
#         [1] events per iteration
#         [2..] ms reported for each pass, in order
#
sub measure1 {
    # run passes
    my @t = callProg(shift); #"$program $args $argsAndTest");
    my @ms = ();
    my @b; # scratch
    for my $a (@t) {
        # $a->[0]: method name, corresponds to $method
        # $a->[1]: 'begin' data, == $iterCount
        # $a->[2]: 'end' data, of the form <ms> <eventsPerIter>
        # $a->[3...]: gc messages from JVM during pass
        @b = split(/\s+/, $a->[2]);
        #push(@ms, $b[0]);
        push(@ms, shift(@b));
    }
    my $iterCount = shift(@b);
    my $operationsPerIter = shift(@b);
    my $eventsPerIter;
    if($#b >= 0) {
      $eventsPerIter = shift(@b);
    }

#    out("Iterations per pass: $iterCount<BR>\n");
#    out("Events per iteration: $eventsPerIter<BR>\n");
#    debug("Iterations per pass: $iterCount<BR>\n");
#    if($eventsPerIter) {
#      debug("Events per iteration: $eventsPerIter<BR>\n");
#    }

    my @ms_str = @ms;
    $ms_str[0] .= " (discarded)" if (@ms_str > 1);
#    out("Raw times (ms/pass): ", join(", ", @ms_str), "<BR>\n");
    debug("Raw times (ms/pass): ", join(", ", @ms_str), "<BR>\n");
    if($eventsPerIter) {
      ($iterCount, $operationsPerIter, \@ms, $eventsPerIter);
    } else {
      ($iterCount, $operationsPerIter, \@ms);
    }
}



#---------------------------------------------------------------------
# Measure a given test method with a give test pattern using the
# global run parameters.
#
# @param the method to run
# @param the pattern defining characters to test
# @param if >0 then the number of iterations per pass.  If <0 then
#        (negative of) the number of seconds per pass.
#
# @return a Dataset object, scaled by iterations per pass and
#         events per iteration, to give time per event
#
sub measure2 {
    my @res = measure1(@_);
    my $iterPerPass = shift(@res);
    my $operationsPerIter = shift(@res);
    my @data = @{ shift(@res) };
    my $eventsPerIter = shift(@res);
    

    shift(@data) if (@data > 1); # discard first run

    my $ds = Dataset->new(@data);
    #$ds->setScale(1.0e-3 / ($iterPerPass * $operationsPerIter));
    ($ds, $iterPerPass, $operationsPerIter, $eventsPerIter);
}


#---------------------------------------------------------------------
# Invoke program and capture results, passing it the given parameters.
#
# @param the method to run
# @param the number of iterations, or if negative, the duration
#        in seconds.  If more than on pass is desired, pass in
#        a string, e.g., "100 100 100".
# @param the pattern defining characters to test
#
# @return an array of results.  Each result is an array REF
#         describing one pass.  The array REF contains:
#         ->[0]: The method name as reported
#         ->[1]: The params on the '= <meth> begin ...' line
#         ->[2]: The params on the '= <meth> end ...' line
#         ->[3..]: GC messages from the JVM, if any
#
sub callProg {
    my $cmd = shift;
    #my $pat = shift;
    #my $n = shift;

    #my $cmd = "java -cp c:\\dev\\myicu4j\\classes $TESTCLASS $method $n $pat";
    debug( "[$cmd]\n"); # for debugging
    open(PIPE, "$cmd|") or die "Can't run \"$cmd\"";
    my @out;
    while (<PIPE>) {
        push(@out, $_);
    }
    close(PIPE) or die "Program failed: \"$cmd\"";

    @out = grep(!/^\#/, @out);  # filter out comments

    #debug( "[", join("\n", @out), "]\n");

    my @results;
    my $method = '';
    my $data = [];
    foreach (@out) {
        next unless (/\S/);

        if (/^=\s*(\w+)\s*(\w+)\s*(.*)/) {
            my ($m, $state, $d) = ($1, $2, $3);
            #debug ("$_ => [[$m $state !!!$d!!! $data ]]\n");
            if ($state eq 'begin') {
                die "$method was begun but not finished" if ($method);
                $method = $m;
                push(@$data, $d);
                push(@$data, ''); # placeholder for end data
            } elsif ($state eq 'end') {
                if ($m ne $method) {
                    die "$method end does not match: $_";
                }
                $data->[1] = $d; # insert end data at [1]
                #debug( "#$method:", join(";",@$data), "\n");
                unshift(@$data, $method); # add method to start
                push(@results, $data);
                $method = '';
                $data = [];
            } else {
                die "Can't parse: $_";
            }
        }

        elsif (/^\[/) {
            if ($method) {
                push(@$data, $_);
            } else {
                # ignore extraneous GC notices
            }
        }

        else {
            # die "Can't parse: $_";
        }
    }

    die "$method was begun but not finished" if ($method);

    @results;
}

sub debug  {
  my $message;
  if($DEBUG != 0) {
    foreach $message (@_) {
      print STDERR "$message";
    }
  }
}

sub measure1Alan {
  #Added here, was global
  my $CALIBRATE = 2; # duration in seconds for initial calibration

    my $method = shift;
    my $pat = shift;
    my $iterCount = shift; # actually might be -seconds/pass

    out("<P>Measuring $method using $pat, ");
    if ($iterCount > 0) {
        out("$iterCount iterations/pass, $NUMPASSES passes</P>\n");
    } else {
        out(-$iterCount, " seconds/pass, $NUMPASSES passes</P>\n");
    }

    # is $iterCount actually -seconds?
    if ($iterCount < 0) {

        # calibrate: estimate ms/iteration
        print "Calibrating...";
        my @t = callJava($method, $pat, -$CALIBRATE);
        print "done.\n";

        my @data = split(/\s+/, $t[0]->[2]);
        my $timePerIter = 1.0e-3 * $data[0] / $data[2];
    
        # determine iterations/pass
        $iterCount = int(-$iterCount / $timePerIter + 0.5);

        out("<P>Calibration pass ($CALIBRATE sec): ");
        out("$data[0] ms, ");
        out("$data[2] iterations = ");
        out(formatSeconds(4, $timePerIter), "/iteration<BR>\n");
    }
    
    # run passes
    print "Measuring $iterCount iterations x $NUMPASSES passes...";
    my @t = callJava($method, $pat, "$iterCount " x $NUMPASSES);
    print "done.\n";
    my @ms = ();
    my @b; # scratch
    for my $a (@t) {
        # $a->[0]: method name, corresponds to $method
        # $a->[1]: 'begin' data, == $iterCount
        # $a->[2]: 'end' data, of the form <ms> <eventsPerIter>
        # $a->[3...]: gc messages from JVM during pass
        @b = split(/\s+/, $a->[2]);
        push(@ms, $b[0]);
    }
    my $eventsPerIter = $b[1];

    out("Iterations per pass: $iterCount<BR>\n");
    out("Events per iteration: $eventsPerIter<BR>\n");

    my @ms_str = @ms;
    $ms_str[0] .= " (discarded)" if (@ms_str > 1);
    out("Raw times (ms/pass): ", join(", ", @ms_str), "<BR>\n");

    ($iterCount, $eventsPerIter, @ms);
}


1;

#eof
