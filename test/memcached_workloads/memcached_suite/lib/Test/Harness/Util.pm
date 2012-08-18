package Test::Harness::Util;

use strict;
use vars qw($VERSION);
$VERSION = '0.01';

use File::Spec;
use Exporter;
use vars qw( @ISA @EXPORT @EXPORT_OK );

@ISA = qw( Exporter );
@EXPORT = ();
@EXPORT_OK = qw( all_in shuffle blibdirs );

=head1 NAME

Test::Harness::Util - Utility functions for Test::Harness::*

=head1 SYNOPSIS

Utility functions for Test::Harness::*

=head1 PUBLIC FUNCTIONS

The following are all available to be imported to your module.  No symbols
are exported by default.

=head2 all_in( {parm => value, parm => value} )

Finds all the F<*.t> in a directory.  Knows to skip F<.svn> and F<CVS>
directories.

Valid parms are:

=over

=item start

Starting point for the search.  Defaults to ".".

=item recurse

Flag to say whether it should recurse.  Default to true.

=back

=cut

sub all_in {
    my $parms = shift;
    my %parms = (
        start => ".",
        recurse => 1,
        %$parms,
    );

    my @hits = ();
    my $start = $parms{start};

    local *DH;
    if ( opendir( DH, $start ) ) {
        my @files = sort readdir DH;
        closedir DH;
        for my $file ( @files ) {
            next if $file eq File::Spec->updir || $file eq File::Spec->curdir;
            next if $file eq ".svn";
            next if $file eq "CVS";

            my $currfile = File::Spec->catfile( $start, $file );
            if ( -d $currfile ) {
                push( @hits, all_in( { %parms, start => $currfile } ) ) if $parms{recurse};
            }
            else {
                push( @hits, $currfile ) if $currfile =~ /\.t$/;
            }
        }
    }
    else {
        warn "$start: $!\n";
    }

    return @hits;
}

=head1 shuffle( @list )

Returns a shuffled copy of I<@list>.

=cut

sub shuffle {
    # Fisher-Yates shuffle
    my $i = @_;
    while ($i) {
        my $j = rand $i--;
        @_[$i, $j] = @_[$j, $i];
    }
}


=head2 blibdir()

Finds all the blib directories.  Stolen directly from blib.pm

=cut

sub blibdirs {
    my $dir = File::Spec->curdir;
    if ($^O eq 'VMS') {
        ($dir = VMS::Filespec::unixify($dir)) =~ s-/\z--;
    }
    my $archdir = "arch";
    if ( $^O eq "MacOS" ) {
        # Double up the MP::A so that it's not used only once.
        $archdir = $MacPerl::Architecture = $MacPerl::Architecture;
    }

    my $i = 5;
    while ($i--) {
        my $blib      = File::Spec->catdir( $dir, "blib" );
        my $blib_lib  = File::Spec->catdir( $blib, "lib" );
        my $blib_arch = File::Spec->catdir( $blib, $archdir );

        if ( -d $blib && -d $blib_arch && -d $blib_lib ) {
            return ($blib_arch,$blib_lib);
        }
        $dir = File::Spec->catdir($dir, File::Spec->updir);
    }
    warn "$0: Cannot find blib\n";
    return;
}

1;
