//=======================================================================
// Copyright 2002 Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/adjacency_list.hpp>
using namespace boost;

// Check to make you can apply a vertex filter with the
// make_filtered_graph function, to fix bug #480175.

struct NotMuchOfAFilter
{
    template<class Vertex> bool operator()(Vertex key)
    const { return true; }
};

int main()
{
    adjacency_list<> graph;
    make_filtered_graph(graph, keep_all(), NotMuchOfAFilter());
}
