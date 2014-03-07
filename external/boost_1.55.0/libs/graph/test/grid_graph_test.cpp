//=======================================================================
// Copyright 2009 Trustees of Indiana University.
// Authors: Michael Hansen, Andrew Lumsdaine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <fstream>
#include <iostream>
#include <set>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/grid_graph.hpp>
#include <boost/random.hpp>
#include <boost/test/minimal.hpp>

using namespace boost;

// Function that prints a vertex to std::cout
template <typename Vertex>
void print_vertex(Vertex vertex_to_print) {

  std::cout << "(";

  for (std::size_t dimension_index = 0;
       dimension_index < vertex_to_print.size();
       ++dimension_index) {
    std::cout << vertex_to_print[dimension_index];

    if (dimension_index != (vertex_to_print.size() - 1)) {
      std::cout << ", ";
    }
  }

  std::cout << ")";
}

template <unsigned int Dims>
void do_test(minstd_rand& generator) {
  typedef grid_graph<Dims> Graph;
  typedef typename graph_traits<Graph>::vertices_size_type vertices_size_type;
  typedef typename graph_traits<Graph>::edges_size_type edges_size_type;

  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;

  std::cout << "Dimensions: " << Dims << ", lengths: ";

  // Randomly generate the dimension lengths (3-10) and wrapping
  boost::array<vertices_size_type, Dims> lengths;
  boost::array<bool, Dims> wrapped;

  for (unsigned int dimension_index = 0;
       dimension_index < Dims;
       ++dimension_index) {
    lengths[dimension_index] = 3 + (generator() % 8);
    wrapped[dimension_index] = ((generator() % 2) == 0);

    std::cout << lengths[dimension_index] <<
      (wrapped[dimension_index] ? " [W]" : " [U]") << ", ";
  }

  std::cout << std::endl;

  Graph graph(lengths, wrapped);

  // Verify dimension lengths and wrapping
  for (unsigned int dimension_index = 0;
       dimension_index < Dims;
       ++dimension_index) {
    BOOST_REQUIRE(graph.length(dimension_index) == lengths[dimension_index]);
    BOOST_REQUIRE(graph.wrapped(dimension_index) == wrapped[dimension_index]);
  }

  // Verify matching indices
  for (vertices_size_type vertex_index = 0;
       vertex_index < num_vertices(graph);
       ++vertex_index) {
    BOOST_REQUIRE(get(boost::vertex_index, graph, vertex(vertex_index, graph)) == vertex_index);
  }

  for (edges_size_type edge_index = 0;
       edge_index < num_edges(graph);
       ++edge_index) {

    edge_descriptor current_edge = edge_at(edge_index, graph);
    BOOST_REQUIRE(get(boost::edge_index, graph, current_edge) == edge_index);
  }

  // Verify all vertices are within bounds
  vertices_size_type vertex_count = 0;
  BOOST_FOREACH(vertex_descriptor current_vertex, vertices(graph)) {

    vertices_size_type current_index =
      get(boost::vertex_index, graph, current_vertex);

    for (unsigned int dimension_index = 0;
         dimension_index < Dims;
         ++dimension_index) {
      BOOST_REQUIRE(/*(current_vertex[dimension_index] >= 0) && */ // Always true
                   (current_vertex[dimension_index] < lengths[dimension_index]));        
    }

    // Verify out-edges of this vertex
    edges_size_type out_edge_count = 0;
    std::set<vertices_size_type> target_vertices;

    BOOST_FOREACH(edge_descriptor out_edge,
                  out_edges(current_vertex, graph)) {

      target_vertices.insert
        (get(boost::vertex_index, graph, target(out_edge, graph)));

      ++out_edge_count;
    }

    BOOST_REQUIRE(out_edge_count == out_degree(current_vertex, graph));

    // Verify in-edges of this vertex
    edges_size_type in_edge_count = 0;

    BOOST_FOREACH(edge_descriptor in_edge,
                  in_edges(current_vertex, graph)) {

      BOOST_REQUIRE(target_vertices.count
                   (get(boost::vertex_index, graph, source(in_edge, graph))) > 0);

      ++in_edge_count;
    }

    BOOST_REQUIRE(in_edge_count == in_degree(current_vertex, graph));

    // The number of out-edges and in-edges should be the same
    BOOST_REQUIRE(degree(current_vertex, graph) ==
                 out_degree(current_vertex, graph) +
                 in_degree(current_vertex, graph));

    // Verify adjacent vertices to this vertex
    vertices_size_type adjacent_count = 0;

    BOOST_FOREACH(vertex_descriptor adjacent_vertex,
                  adjacent_vertices(current_vertex, graph)) {

      BOOST_REQUIRE(target_vertices.count
                   (get(boost::vertex_index, graph, adjacent_vertex)) > 0);

      ++adjacent_count;
    }

    BOOST_REQUIRE(adjacent_count == out_degree(current_vertex, graph));

    // Verify that this vertex is not listed as connected to any
    // vertices outside of its adjacent vertices.
    BOOST_FOREACH(vertex_descriptor unconnected_vertex, vertices(graph)) {
      
      vertices_size_type unconnected_index =
        get(boost::vertex_index, graph, unconnected_vertex);

      if ((unconnected_index == current_index) ||
          (target_vertices.count(unconnected_index) > 0)) {
        continue;
      }

      BOOST_REQUIRE(!edge(current_vertex, unconnected_vertex, graph).second);
      BOOST_REQUIRE(!edge(unconnected_vertex, current_vertex, graph).second);
    }

    ++vertex_count;
  }

  BOOST_REQUIRE(vertex_count == num_vertices(graph));

  // Verify all edges are within bounds
  edges_size_type edge_count = 0;
  BOOST_FOREACH(edge_descriptor current_edge, edges(graph)) {

    vertices_size_type source_index =
      get(boost::vertex_index, graph, source(current_edge, graph));

    vertices_size_type target_index =
      get(boost::vertex_index, graph, target(current_edge, graph));

    BOOST_REQUIRE(source_index != target_index);
    BOOST_REQUIRE(/* (source_index >= 0) : always true && */ (source_index < num_vertices(graph)));
    BOOST_REQUIRE(/* (target_index >= 0) : always true && */ (target_index < num_vertices(graph)));

    // Verify that the edge is listed as existing in both directions
    BOOST_REQUIRE(edge(source(current_edge, graph), target(current_edge, graph), graph).second);
    BOOST_REQUIRE(edge(target(current_edge, graph), source(current_edge, graph), graph).second);

    ++edge_count;
  }

  BOOST_REQUIRE(edge_count == num_edges(graph));
}

int test_main(int argc, char* argv[]) {

  std::size_t random_seed = time(0);

  if (argc > 1) {
    random_seed = lexical_cast<std::size_t>(argv[1]);
  }

  minstd_rand generator(random_seed);

  do_test<0>(generator);
  do_test<1>(generator);
  do_test<2>(generator);
  do_test<3>(generator);
  do_test<4>(generator);

  return (0);
}

