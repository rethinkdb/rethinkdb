#!/bin/sh
# \
exec tclsh "$0" "$@"

# Copyright (C) 2009 The Trustees of Indiana University.
# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Authors: Jeremiah Willcock, Andrew Lumsdaine

foreach input [glob *.rst] {
  set output [file join html "[file rootname $input].html"]
  puts "Processing $input -> $output"
  set processor [open "|rst2html.py --stylesheet=../../../../rst.css -gdt --link-stylesheet --traceback --trim-footnote-reference-space --footnote-references=superscript >$output" w]
  set inputfd [open $input r]
  set data [read $inputfd]
  close $inputfd
  foreach line [split $data \n] {
    if {[regexp {^\.\. image:: (http:.*)$} $line _ url]} {
      set tag $url
      regsub -all {.*/} $tag {} tag
      regsub -all {[^a-zA-Z0-9]} $tag _ tag
      set imageoutput [file join html "$tag.png"]
      puts "Getting image $url -> $imageoutput"
      exec wget -q -O $imageoutput $url
      puts $processor ".. image:: [file tail $imageoutput]"
    } else {
      puts $processor $line
    }
  }
  close $processor
}
