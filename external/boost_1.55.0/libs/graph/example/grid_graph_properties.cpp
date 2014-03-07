//=======================================================================
// Copyright 2012 David Doria
// Authors: David Doria
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>
#include <boost/array.hpp>
#include <boost/graph/grid_graph.hpp>

int main(int argc, char* argv[]) 
{
  // A 2D grid graph
  typedef boost::grid_graph<2> GraphType;

  // Create a 5x5 graph
  const unsigned int dimension = 5;
  boost::array<std::size_t, 2> lengths = { { dimension, dimension } };
  GraphType graph(lengths);

  // Get the index map of the grid graph
  typedef boost::property_map<GraphType, boost::vertex_index_t>::const_type indexMapType;
  indexMapType indexMap(get(boost::vertex_index, graph));

  // Create a float for every node in the graph
  boost::vector_property_map<float, indexMapType> dataMap(num_vertices(graph), indexMap);

  // Associate the value 2.0 with the node at position (0,1) in the grid
  boost::graph_traits<GraphType>::vertex_descriptor v = { { 0, 1 } };
  put(dataMap, v, 2.0f);

  // Get the data at the node at position (0,1) in the grid
  float retrieved = get(dataMap, v);
  std::cout << "Retrieved value: " << retrieved << std::endl;

  return 0;
}
