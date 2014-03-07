# Copyright (c) 2002 Trustees of Indiana University
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)


$lastpage = 0;
$thispage = 1;
$counter =  1;
$alphabet = "\@abcdefghijklmnopqrstuvwxyz";
$Alphabet = "\@ABCDEFGHIJKLMNOPQRSTUVWXYZ";
$out = "";
$saved_full = "";
$saved_empty = "";

while(<>) {

# These changes are so that it works when we aren't using hyperref

#  if (/(\\newlabel.*\{\{)([0-9]+)(\}\{)([0-9ivx]+)(\}.*JWebCtr\.)([0-9]+)(.*)/) {
  if (/\\newlabel\{sec:.*/) {
    # make sure not to munge normal (non jweb part) section labels
    print ;
  } elsif (/\\newlabel\{class:.*/) {
    # make sure not to munge normal (non jweb part) class labels
    print ;
  } elsif (/\\newlabel\{tab:.*/) {
    # make sure not to munge normal (non jweb part) table labels
    print ;
  } elsif (/\\newlabel\{concept:.*/) {
    # make sure not to munge normal (non jweb part) concept labels
    print ;
  } elsif (/\\newlabel\{fig:.*/) {
    # make sure not to munge normal (non jweb part) class labels
    print ;
  } elsif (/(\\newlabel.*\{\{)([0-9\.]+)(\}\{)([0-9ivx]+)(\}.*)(.*)/) {
    $thispage = $4;

    if ($thispage ne $lastpage) {
      
      $counter = 1;

      print $saved_empty;

#      $saved_full = "$1".substr($alphabet,$counter,1)."$3$4$5$6$7\n";
#      $saved_empty = "$1"."$3$4$5$6$7\n";
      $saved_full = "$1".substr($alphabet,$counter,1)."$3$4$5\n";
      $saved_empty = "$1"."$3$4$5\n";

    } else {
      print $saved_full;
#      print "$1".substr($alphabet,$counter,1)."$3$4$5$counter$7\n";
      print "$1".substr($alphabet,$counter,1)."$3$4$5\n";
      $saved_full = "";
      $saved_empty = "";
    }

    $lastpage = $thispage;
    $counter++;

  } else {
    print ;
  }
}
print $saved_empty;



# get a line
# cases
# - ref
#   - if it is first, save off someplace
#     - if there is a first saved, dump the empty version
#   - else
#     - if there is a first saved, dump the non empty version
# - not a ref
