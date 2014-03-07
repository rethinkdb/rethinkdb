# -*- makefile -*-
# Copyright (c) 2002 Trustees of Indiana University
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

.SUFFIXES:  .fig .gif .tif .jpeg

.fig.gif:
	fig2dev -L gif $*.fig > $*.gif

.fig.tif:
	fig2dev -L tiff $*.fig > $*.tif

.fig.jpeg:
	fig2dev -L jpeg $*.fig > $*.jpg



FIG	= \
analogy.fig       dfs_example.fig             quick_start.fig \
back_edges.fig    dfs_family.fig              stl_iter.fig    \
bfs_example.fig   dfs_visitor.fig             tree_edges.fig  \
bfs_family.fig    disjoint_set_family.fig     visitor.fig     \
bfs_visitor.fig   file_dep.fig					    \
concepts.fig      forward_or_cross_edges.fig  digraph.fig     \
undigraph.fig     adj_matrix.fig              adj_list.fig    \
edge_list.fig     dfs.fig                     knights_tour.fig \
search_states.fig graph_search.fig


GIF	= $(FIG:.fig=.gif)
TIFF	= $(FIG:.fig=.tif)
JPEG	= $(FIG:.fig=.jpg)

gifs: $(GIF)
tiffs: $(TIFF)
jpegs: $(JPEG)

clean:
	/bin/rm -f *.gif *.tif *.jpg



