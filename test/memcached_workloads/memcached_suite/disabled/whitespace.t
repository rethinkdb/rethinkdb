#!/usr/bin/perl
use strict;
use FindBin qw($Bin);
our @files;

BEGIN {
    chdir "$Bin/.." or die;

    my @exempted = qw(Makefile.am ChangeLog doc/Makefile.am);
    push(@exempted, glob("doc/*.xml"));
    push(@exempted, glob("doc/xml2rfc/*.xsl"));
    push(@exempted, glob("m4/*backport*m4"));
    my %exempted_hash = map { $_ => 1 } @exempted;

    my @stuff = split /\0/, `git ls-files -z -c -m -o --exclude-standard`;
    @files = grep { ! $exempted_hash{$_} } @stuff;

    # We won't find any files if git isn't installed.  If git isn't
    # installed, they're probably not doing any useful development, or
    # at the very least am will clean up whitespace when we receive
    # their patch.
    unless (@files) {
        use Test::More;
        plan skip_all => "Skipping tests probably because you don't have git.";
        exit 0;
    }
}

use Test::More tests => scalar(@files);

foreach my $f (@files) {
    open(my $fh, $f) or die;
    my $before = do { local $/; <$fh>; };
    close ($fh);
    my $after = $before;
    $after =~ s/\t/    /g;
    $after =~ s/ +$//mg;
    $after .= "\n" unless $after =~ /\n$/;
    ok ($after eq $before, "$f (see devtools/clean-whitespace.pl)");
}
