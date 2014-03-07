#!/bin/sh

# Copyright (C) 2009 The Trustees of Indiana University.
# Copyright (C) 2010 Daniel Trebbien.
# Use, modification and distribution is subject to the Boost Software
# License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Authors: Jeremiah Willcock, Daniel Trebbien, Andrew Lumsdaine

for i in read_graphml read_graphviz write_graphml; do
  rst2html.py -gdt --link-stylesheet --traceback --trim-footnote-reference-space --footnote-references=superscript --stylesheet=../../../rst.css $i.rst > $i.html
done
# Also see grid_graph_export_png.sh for figure conversions

# Stoer-Wagner images from Daniel Trebbien
fdp -s -n -Tgif -ostoer_wagner_imgs/digraph1.gif stoer_wagner_imgs/digraph1.dot
fdp -s -n -Tgif -ostoer_wagner_imgs/digraph1-min-cut.gif stoer_wagner_imgs/digraph1-min-cut.dot
fdp -s -n -Tgif -ostoer_wagner_imgs/stoer_wagner-example.gif stoer_wagner_imgs/stoer_wagner-example.dot
fdp -s -n -Tgif -ostoer_wagner_imgs/stoer_wagner-example-c1.gif stoer_wagner_imgs/stoer_wagner-example-c1.dot
fdp -s -n -Tgif -ostoer_wagner_imgs/stoer_wagner-example-min-cut.gif stoer_wagner_imgs/stoer_wagner-example-min-cut.dot
dot -Tgif -ostoer_wagner_imgs/stoer_wagner.cpp.gif stoer_wagner_imgs/stoer_wagner.cpp.dot
