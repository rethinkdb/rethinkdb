//=======================================================================
// Copyright 2009 Trustees of Indiana University.
// Authors: Michael Hansen, Andrew Lumsdaine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>
#include <boost/array.hpp>
#include <boost/graph/grid_graph.hpp>

#define DIMENSIONS 3
using namespace boost;

typedef grid_graph<DIMENSIONS> Graph;
typedef graph_traits<Graph> Traits;

// Define a simple function to print vertices
void print_vertex(Traits::vertex_descriptor vertex_to_print) {
  std::cout << "(" << vertex_to_print[0] << ", " << vertex_to_print[1] <<
    ", " << vertex_to_print[2] << ")" << std::endl;
}

int main(int argc, char* argv[]) {

  // Define a 3x5x7 grid_graph where the second dimension doesn't wrap
  boost::array<std::size_t, 3> lengths = { { 3, 5, 7 } };
  boost::array<bool, 3> wrapped = { { true, false, true } };
  Graph graph(lengths, wrapped);

  // Do a round-trip test of the vertex index functions
  for (Traits::vertices_size_type v_index = 0;
       v_index < num_vertices(graph); ++v_index) {

    // The two indicies should always be equal
    std::cout << "Index of vertex " << v_index << " is " <<
      get(boost::vertex_index, graph, vertex(v_index, graph)) << std::endl;

  }

  // Do a round-trip test of the edge index functions
  for (Traits::edges_size_type e_index = 0;
       e_index < num_edges(graph); ++e_index) {

    // The two indicies should always be equal
    std::cout << "Index of edge " << e_index << " is " <<
      get(boost::edge_index, graph, edge_at(e_index, graph)) << std::endl;

  }

  // Print number of dimensions
  std::cout << graph.dimensions() << std::endl; // prints "3"

  // Print dimension lengths (same order as in the lengths array)
  std::cout << graph.length(0) << "x" << graph.length(1) <<
    "x" << graph.length(2) << std::endl; // prints "3x5x7"

  // Print dimension wrapping (W = wrapped, U = unwrapped)
  std::cout << (graph.wrapped(0) ? "W" : "U") << ", " <<
    (graph.wrapped(1) ? "W" : "U") << ", " <<
    (graph.wrapped(2) ? "W" : "U") << std::endl; // prints "W, U, W"

  // Start with the first vertex in the graph
  Traits::vertex_descriptor first_vertex = vertex(0, graph);
  print_vertex(first_vertex); // prints "(0, 0, 0)"

  // Print the next vertex in dimension 0
  print_vertex(graph.next(first_vertex, 0)); // prints "(1, 0, 0)"

  // Print the next vertex in dimension 1
  print_vertex(graph.next(first_vertex, 1)); // prints "(0, 1, 0)"

  // Print the 5th next vertex in dimension 2
  print_vertex(graph.next(first_vertex, 2, 5)); // prints "(0, 0, 4)"

  // Print the previous vertex in dimension 0 (wraps)
  print_vertex(graph.previous(first_vertex, 0)); // prints "(2, 0, 0)"

  // Print the previous vertex in dimension 1 (doesn't wrap, so it's the same)
  print_vertex(graph.previous(first_vertex, 1)); // prints "(0, 0, 0)"

  // Print the 20th previous vertex in dimension 2 (wraps around twice)
  print_vertex(graph.previous(first_vertex, 2, 20)); // prints "(0, 0, ?)"
}
