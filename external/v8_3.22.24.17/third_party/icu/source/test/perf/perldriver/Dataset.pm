#!/usr/local/bin/perl
#  ********************************************************************
#  * COPYRIGHT:
#  * Copyright (c) 2002, International Business Machines Corporation and
#  * others. All Rights Reserved.
#  ********************************************************************

package Dataset;
use Statistics::Descriptive;
use Statistics::Distributions;
use strict;

# Create a new Dataset with the given data.
sub new {
    my ($class) = shift;
    my $self = bless {
        _data => \@_,
        _scale => 1.0,
        _mean => 0.0,
        _error => 0.0,
    }, $class;

    my $n = @_;
    
    if ($n >= 1) {
        my $stats = Statistics::Descriptive::Full->new();
        $stats->add_data(@{$self->{_data}});
        $self->{_mean} = $stats->mean();

        if ($n >= 2) {
            # Use a t distribution rather than Gaussian because (a) we
            # assume an underlying normal dist, (b) we do not know the
            # standard deviation -- we estimate it from the data, and (c)
            # we MAY have a small sample size (also works for large n).
            my $t = Statistics::Distributions::tdistr($n-1, 0.005);
            $self->{_error} = $t * $stats->standard_deviation();
        }
    }

    $self;
}

# Set a scaling factor for all data; 1.0 means no scaling.
# Scale must be > 0.
sub setScale {
    my ($self, $scale) = @_;
    $self->{_scale} = $scale;
}

# Multiply the scaling factor by a value.
sub scaleBy {
    my ($self, $a) = @_;
    $self->{_scale} *= $a;
}

# Return the mean.
sub getMean {
    my $self = shift;
    return $self->{_mean} * $self->{_scale};
}

# Return a 99% error based on the t distribution.  The dataset
# is desribed as getMean() +/- getError().
sub getError {
    my $self = shift;
    return $self->{_error} * $self->{_scale};
}

# Divide two Datasets and return a new one, maintaining the
# mean+/-error.  The new Dataset has no data points.
sub divide {
    my $self = shift;
    my $rhs = shift;
    
    my $minratio = ($self->{_mean} - $self->{_error}) /
                   ($rhs->{_mean} + $rhs->{_error});
    my $maxratio = ($self->{_mean} + $self->{_error}) /
                   ($rhs->{_mean} - $rhs->{_error});

    my $result = Dataset->new();
    $result->{_mean} = ($minratio + $maxratio) / 2;
    $result->{_error} = $result->{_mean} - $minratio;
    $result->{_scale} = $self->{_scale} / $rhs->{_scale};
    $result;
}

# subtracts two Datasets and return a new one, maintaining the
# mean+/-error.  The new Dataset has no data points.
sub subtract {
    my $self = shift;
    my $rhs = shift;
    
    my $result = Dataset->new();
    $result->{_mean} = $self->{_mean} - $rhs->{_mean};
    $result->{_error} = $self->{_error} + $rhs->{_error};
    $result->{_scale} = $self->{_scale};
    $result;
}

# adds two Datasets and return a new one, maintaining the
# mean+/-error.  The new Dataset has no data points.
sub add {
    my $self = shift;
    my $rhs = shift;
    
    my $result = Dataset->new();
    $result->{_mean} = $self->{_mean} + $rhs->{_mean};
    $result->{_error} = $self->{_error} + $rhs->{_error};
    $result->{_scale} = $self->{_scale};
    $result;
}

# Divides a dataset by a scalar.
# The new Dataset has no data points.
sub divideByScalar {
    my $self = shift;
    my $s = shift;
    
    my $result = Dataset->new();
    $result->{_mean} = $self->{_mean}/$s;
    $result->{_error} = $self->{_error}/$s;
    $result->{_scale} = $self->{_scale};
    $result;
}

# Divides a dataset by a scalar.
# The new Dataset has no data points.
sub multiplyByScalar {
    my $self = shift;
    my $s = shift;
    
    my $result = Dataset->new();
    $result->{_mean} = $self->{_mean}*$s;
    $result->{_error} = $self->{_error}*$s;
    $result->{_scale} = $self->{_scale};
    $result;
}

1;
