// Copyright 2010 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Jeremiah Willcock
//           Andrew Lumsdaine

#include <boost/graph/random_spanning_tree.hpp>
#include <boost/graph/grid_graph.hpp>
#include <boost/array.hpp>
#include <boost/random.hpp>
#include <boost/property_map/shared_array_property_map.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/property_maps/constant_property_map.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <boost/graph/iteration_macros.hpp>

using namespace boost;
using namespace std;

typedef grid_graph<2> graph_type;
typedef graph_traits<graph_type> gt;

template <typename Graph, typename PredMap, typename WeightMap>
void write_spanning_tree(const Graph& g, PredMap pred, WeightMap weight, string filename) {
  shared_array_property_map<string, typename property_map<Graph, edge_index_t>::const_type> edge_style(num_edges(g), get(edge_index, g));
  shared_array_property_map<string, typename property_map<Graph, vertex_index_t>::const_type> vertex_pos(num_vertices(g), get(vertex_index, g));
  BGL_FORALL_EDGES_T(e, g, Graph) {
    put(edge_style, e, (get(pred, target(e, g)) == source(e, g) || get(pred, source(e, g)) == target(e, g)) ? "bold" : "dotted");
  }
  BGL_FORALL_VERTICES_T(v, g, Graph) {
    put(vertex_pos, v, lexical_cast<string>(v[0]) + "," + lexical_cast<string>(v[1]));
  }
  dynamic_properties dp;
  dp.property("style", edge_style);
  dp.property("pos", vertex_pos);
  dp.property("len", weight);
  dp.property("node_id", get(vertex_index, g));
  ofstream out(filename.c_str());
  write_graphviz_dp(out, g, dp);
}

int main(int, char**) {

  boost::array<size_t, 2> sizes = {{ 5, 5 }};
  graph_type g(sizes);

  shared_array_property_map<gt::vertex_descriptor, property_map<graph_type, vertex_index_t>::const_type> pred(num_vertices(g), get(vertex_index, g));
  shared_array_property_map<double, property_map<graph_type, edge_index_t>::const_type> weight(num_edges(g), get(edge_index, g));

  BGL_FORALL_EDGES(e, g, graph_type) {put(weight, e, (1. + get(edge_index, g, e)) / num_edges(g));}

  boost::mt19937 gen;
  random_spanning_tree(g, gen, predecessor_map(pred));
  // write_spanning_tree(g, pred, constant_property_map<gt::edge_descriptor, double>(1.), "unweight_random_st.dot");
  random_spanning_tree(g, gen, predecessor_map(pred));
  // write_spanning_tree(g, pred, constant_property_map<gt::edge_descriptor, double>(1.), "unweight_random_st2.dot");

  random_spanning_tree(g, gen, predecessor_map(pred).weight_map(weight));
  // write_spanning_tree(g, pred, weight, "weight_random_st.dot");

  return 0;
}
