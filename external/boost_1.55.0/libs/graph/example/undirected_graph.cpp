//=======================================================================
// Copyright 2012
// Authors: David Doria
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/undirected_graph.hpp>

typedef boost::undirected_graph<boost::no_property> Graph;

int main(int,char*[])
{
  // Create a graph object
  Graph g;

  // Add vertices
  boost::graph_traits<Graph>::vertex_descriptor v0 = g.add_vertex();
  boost::graph_traits<Graph>::vertex_descriptor v1 = g.add_vertex();
  boost::graph_traits<Graph>::vertex_descriptor v2 = g.add_vertex();

  // Add edges
  g.add_edge(v0, v1);
  g.add_edge(v1, v2);

  return 0;
}
