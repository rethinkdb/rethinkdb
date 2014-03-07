#!/usr/bin/perl -w
# Bart Garst - 7/1/2004 
# additional comments at bottom of file

#############################################################################
#  Copyright (c) 2001-2005 CrystalClear Software, Inc.                      #
#  Subject to the Boost Software License, Version 1.0.                      #
#  (See accompanying file LICENSE_1_0.txt or  http://www.boost.org/LICENSE_1_0.txt) #
#############################################################################

use strict;

# key-value of file name and id attribute
# if the attributes are changed here it would be a good idea to
# also change the links in doxy.xml
my %files = (
  'date_time_autodoc.boostbook'  => 'date_time_reference',
  'gregorian_autodoc.boostbook'  => 'gregorian_reference',
  'posix_time_autodoc.boostbook' => 'posix_time_reference',
  'local_time_autodoc.boostbook' => 'local_time_reference'
);


foreach my $key(keys %files) {
  rewrite_tags($key, $files{$key});
}

exit;

### subroutines ###

# separate words at underscores and capitalize first letter of each
sub make_title {
  my $a = shift || die "Missing required parameter to make_title()\n";
  my @wrds = split(/_/, $a); # remove underscores
  foreach(@wrds){
    $_ = "\u$_"; # capitalize first letter
  }
  $a = join(" ",@wrds);

  return $a;
}

sub rewrite_tags {
  my $filename = shift || die "Error: argument 1 missing to sub $!";
  my $id_tag = shift || die "Error: argument 2 missing to sub $!";
  my ($line, @new_file, $title, $processed);
  
  $processed = 1; # has this file already been processed?
  
  print "...processing $filename...\n";

  # prepare a title from id attribute
  $title = make_title($id_tag);
  
  # open & read file and change appropriate line
  open(INP, "<$filename") || die "File open (read) failed: $!";
  while($line = <INP>){
    if($line =~ /<library-reference>/) {
      push(@new_file, "<section id=\"$id_tag\">\n");
      push(@new_file, "<title>$title</title>\n");
      $processed = 0; # file had not been previously processed 
    }
    elsif($line =~ /<\/library-reference>/) {
      push(@new_file, "</section>\n");
    }
    else{
      push(@new_file, $line);
    }
  }
  close(INP);
  
  # open & write new file w/ same name
  open(OTP, ">$filename") || die "File open (write) failed: $!";
  print OTP shift(@new_file);

  if($processed == 0){ # has not been previously processed so add license
    my($day, $year) = (localtime)[3,5];
    my $month = (qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec))[(localtime)[4]];
    $year += 1900; # adjust year
    print OTP <<EO_LIC;
<!-- Copyright (c) 2001-2005 CrystalClear Software, Inc.
     Subject to the Boost Software License, Version 1.0. 
     (See accompanying file LICENSE_1_0.txt or  http://www.boost.org/LICENSE_1_0.txt)
-->
<!-- date source directory processed: $year-$month-$day -->
EO_LIC
  }

  foreach(@new_file){
    print OTP "$_";
  }
  close(OTP);
}

__END__

Rewrites the library-reference tagset as a section tagset and adds
a title to the generated *.boostbook files. It will NOT update a 
file that has already been rewritten.

Change log
7/19/2004
        - rewrite library-reference tags as section tags and add title tags
        - renamed fix_id sub to rewrite_tags.
8/31/2004
        - added license to this file and writes license to boostbook files
11/01/2004
        - fixed minor bug that placed multiple license statements in files if
          input file had already had it's tags fixed.
        - added a processed date to the license statement
12/02/2005
        - added local_time_autodoc.boostbook
        - updated copyrights to 2005
