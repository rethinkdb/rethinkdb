#!/bin/bash

#=======================================================================
# Copyright 2009 Trustees of Indiana University.
# Authors: Michael Hansen, Andrew Lumsdaine
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http:#www.boost.org/LICENSE_1_0.txt)
#=======================================================================

# Unindexed, unwrapped
inkscape --export-png grid_graph_unwrapped.png --export-id g3150 --export-id-only grid_graph_unindexed.svg

# Unindexed, wrapped
inkscape --export-png grid_graph_wrapped.png grid_graph_unindexed.svg

# Indexed, unwrapped
inkscape --export-png grid_graph_indexed.png grid_graph_indexed.svg