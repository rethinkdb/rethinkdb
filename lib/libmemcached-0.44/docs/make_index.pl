#!/usr/bin/perl -w

use strict;
use CGI;

sub main {
  my $cgi = new CGI;

  print $cgi->start_html('Libmemcached Documentation') . "\n";
  print $cgi->h1('Libmemcached Documentation') . "\n";

  print $cgi->a({ href => "libmemcached.html" }, "Introduction to Libmemcached") . $cgi->br() . "\n";
  print $cgi->a({ href => "libmemcached_examples.html" }, "Libmemcached Examples") . $cgi->br() . "\n";
  print $cgi->br() . "\n";
  print $cgi->br() . "\n";

  foreach (@ARGV)
  {
    my $url=  $_;
    my $name= $_;
    $name  =~ s/\.html//g;
    next if $name eq 'index'; 
    next if $name eq 'libmemcached'; 
    next if $name eq 'libmemcached_examples'; 
    print "<li\>" . $cgi->a({ href => $url }, $name) . $cgi->br() . "\n";
  }
  print $cgi->end_html; 
}

main();
