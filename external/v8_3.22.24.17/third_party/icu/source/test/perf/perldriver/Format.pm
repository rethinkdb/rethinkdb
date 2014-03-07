#!/usr/local/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************

my $PLUS_MINUS = "&plusmn;";

#|#---------------------------------------------------------------------
#|# Format a confidence interval, as given by a Dataset.  Output is as
#|# as follows:
#|#   241.23 - 241.98 => 241.5 +/- 0.3
#|#   241.2 - 243.8 => 242 +/- 1
#|#   211.0 - 241.0 => 226 +/- 15 or? 230 +/- 20
#|#   220.3 - 234.3 => 227 +/- 7
#|#   220.3 - 300.3 => 260 +/- 40
#|#   220.3 - 1000 => 610 +/- 390 or? 600 +/- 400
#|#   0.022 - 0.024 => 0.023 +/- 0.001
#|#   0.022 - 0.032 => 0.027 +/- 0.005
#|#   0.022 - 1.000 => 0.5 +/- 0.5
#|# In other words, take one significant digit of the error value and
#|# display the mean to the same precision.
#|sub formatDataset {
#|    my $ds = shift;
#|    my $lower = $ds->getMean() - $ds->getError();
#|    my $upper = $ds->getMean() + $ds->getError();
#|    my $scale = 0;
#|    # Find how many initial digits are the same
#|    while ($lower < 1 ||
#|           int($lower) == int($upper)) {
#|        $lower *= 10;
#|        $upper *= 10;
#|        $scale++;
#|    }
#|    while ($lower >= 10 &&
#|           int($lower) == int($upper)) {
#|        $lower /= 10;
#|        $upper /= 10;
#|        $scale--;
#|    }
#|}

#---------------------------------------------------------------------
# Format a number, optionally with a +/- delta, to n significant
# digits.
#
# @param significant digit, a value >= 1
# @param multiplier
# @param time in seconds to be formatted
# @optional delta in seconds
#
# @return string of the form "23" or "23 +/- 10".
#
sub formatNumber {
    my $sigdig = shift;
    my $mult = shift;
    my $a = shift;
    my $delta = shift; # may be undef
    
    my $result = formatSigDig($sigdig, $a*$mult);
    if (defined($delta)) {
        my $d = formatSigDig($sigdig, $delta*$mult);
        # restrict PRECISION of delta to that of main number
        if ($result =~ /\.(\d+)/) {
            # TODO make this work for values with all significant
            # digits to the left of the decimal, e.g., 1234000.

            # TODO the other thing wrong with this is that it
            # isn't rounding the $delta properly.  Have to put
            # this logic into formatSigDig().
            my $x = length($1);
            $d =~ s/\.(\d{$x})\d+/.$1/;
        }
        $result .= " $PLUS_MINUS " . $d;
    }
    $result;
}

#---------------------------------------------------------------------
# Format a time, optionally with a +/- delta, to n significant
# digits.
#
# @param significant digit, a value >= 1
# @param time in seconds to be formatted
# @optional delta in seconds
#
# @return string of the form "23 ms" or "23 +/- 10 ms".
#
sub formatSeconds {
    my $sigdig = shift;
    my $a = shift;
    my $delta = shift; # may be undef

    my @MULT = (1   , 1e3,  1e6,  1e9);
    my @SUFF = ('s' , 'ms', 'us', 'ns');

    # Determine our scale
    my $i = 0;
    #always do seconds if the following line is commented out
    ++$i while ($a*$MULT[$i] < 1 && $i < @MULT);
    
    formatNumber($sigdig, $MULT[$i], $a, $delta) . ' ' . $SUFF[$i];
}

#---------------------------------------------------------------------
# Format a percentage, optionally with a +/- delta, to n significant
# digits.
#
# @param significant digit, a value >= 1
# @param value to be formatted, as a fraction, e.g. 0.5 for 50%
# @optional delta, as a fraction
#
# @return string of the form "23 %" or "23 +/- 10 %".
#
sub formatPercent {
    my $sigdig = shift;
    my $a = shift;
    my $delta = shift; # may be undef
    
    formatNumber($sigdig, 100, $a, $delta) . '%';
}

#---------------------------------------------------------------------
# Format a number to n significant digits without using exponential
# notation.
#
# @param significant digit, a value >= 1
# @param number to be formatted
#
# @return string of the form "1234" "12.34" or "0.001234".  If
#         number was negative, prefixed by '-'.
#
sub formatSigDig {
    my $n = shift() - 1;
    my $a = shift;

    local $_ = sprintf("%.${n}e", $a);
    my $sign = (s/^-//) ? '-' : '';

    my $a_e;
    my $result;
    if (/^(\d)\.(\d+)e([-+]\d+)$/) {
        my ($d, $dn, $e) = ($1, $2, $3);
        $a_e = $e;
        $d .= $dn;
        $e++;
        $d .= '0' while ($e > length($d));
        while ($e < 1) {
            $e++;
            $d = '0' . $d;
        }
        if ($e == length($d)) {
            $result = $sign . $d;
        } else {
            $result = $sign . substr($d, 0, $e) . '.' . substr($d, $e);
        }
    } else {
        die "Can't parse $_";
    }
    $result;
}

1;

#eof
