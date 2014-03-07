#!/usr/bin/perl -w

# Copyright 2004 Aleksey Gurtovoy 
# Copyright 2001 Jens Maurer 
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) 

use strict;

my $filename;
my $compiler;
my $time = 0;
my $ct = 0;
my $first = 2;

print "<html>\n<head>\n<title>\nCompile Times</title>\n</head>\n\n";
print "<body bgcolor=\"#ffffff\" text=\"#000000\">\n";
print "<img border=\"0\" src=\"boost.png\" width=\"277\" height=\"86\">";
print "<p>\n";
print "Compile time for each successful regression test in seconds.\n";
print "<p>\n";

print "<table border=\"1\">\n";
print "<tr><td>Test</td>\n";

while(<>) {
  if(/^\*\*\* (.*) \*\*\*$/) {
    $filename = $1;
    $first = ($first == 0 ? 0 : $first-1);
    if($first == 0) {
      print "</tr>\n\n<tr align=right>\n<td align=left><a href=\"http://www.boost.org/$filename\">$filename</a></td>\n";
    }
  } elsif(/^\*\* (.*)/) {
    $compiler = $1;
    if($first) {
      print "<td>$compiler</td>\n";
    } else {
      $ct = 1;
    }
  } elsif($ct && /^CPU time: ([.0-9]*) s user, ([.0-9]*) s system/) {
    $time = $1 + $2;
  } elsif($ct && /^Pass$/) {
    printf "<td>%.02f</td>\n", $time;
    $ct = 0; 
  } elsif($ct && /^Fail$/) {
    print "<td>-</td>\n";
    $ct = 0; 
  }  
}

print "</tr>\n";
print "</table>\n";
print "</body>\n</html>\n";

