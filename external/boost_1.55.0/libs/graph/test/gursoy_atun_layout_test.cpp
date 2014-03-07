// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Jeremiah Willcock
//           Douglas Gregor
//           Andrew Lumsdaine
#include <boost/graph/gursoy_atun_layout.hpp>
#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/random.hpp"
#include "boost/graph/graphviz.hpp"
#include "boost/random/mersenne_twister.hpp"
#include "boost/random/linear_congruential.hpp"
#include "boost/random/uniform_01.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

#if 0
#include <boost/graph/plod_generator.hpp>
#include <boost/graph/small_world_generator.hpp>
#endif
using namespace boost;

template <class Property, class Vertex>
struct position_writer {
  const Property& property;

  position_writer(const Property& property): property(property) {}

  void operator()(std::ostream& os, const Vertex& v) const {
    os << "[pos=\"" << int(property[v][0]) << "," << int(property[v][1]) << "\"]";
  }
};

struct graph_writer {
  void operator()(std::ostream& os) const {
    os << "node [shape=point, width=\".01\", height=\".01\", fixedsize=\"true\"]"
       << std::endl;
  }
};

int main(int, char*[]) {
  // Generate a graph structured like a grid, cylinder, or torus; lay it out in
  // a square grid; and output it in dot format

  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                boost::no_property, 
                                boost::property<boost::edge_weight_t, double>
                                > graph_type;
  typedef boost::graph_traits<graph_type>::vertex_descriptor vertex_descriptor;
  // boost::mt19937 rng;
  // boost::generate_random_graph(graph, 100, 600, rng, false, false);

#if 1
  graph_type graph;

  // Make grid, like Gursoy and Atun used
  std::map<int, std::map<int, vertex_descriptor> > verts;
  const int grid_size = 20;
  boost::minstd_rand edge_weight_gen;
  boost::uniform_01<boost::minstd_rand> random_edge_weight(edge_weight_gen);
  for (int i = 0; i < grid_size; ++i)
    for (int j = 0; j < grid_size; ++j)
      verts[i][j] = add_vertex(graph);
  for (int i = 0; i < grid_size; ++i) {
    for (int j = 0; j < grid_size; ++j) {
      if (i != 0)
        add_edge(verts[i][j], verts[i-1][j], random_edge_weight(), graph);
      if (j != 0)
        add_edge(verts[i][j], verts[i][j-1], random_edge_weight(), graph);
#if 0
      // Uncomment parts of this to get a cylinder or torus
      if (i == 0)
        add_edge(verts[0][j], verts[grid_size-1][j], random_edge_weight(), 
                 graph);
      if (j == 0)
        add_edge(verts[i][0], verts[i][grid_size-1], random_edge_weight(), 
                 graph);
#endif
    }
  }
#else
  using namespace boost;

#if 0
  int n = 10000;
  double alpha = 0.4;
  double beta = 50;
  minstd_rand gen;
  graph_type graph(plod_iterator<minstd_rand, graph_type>(gen, n, alpha, beta),
                   plod_iterator<minstd_rand, graph_type>(),
                   n);
#else 
  int n = 1000;
  int k = 6;
  double p = 0.001;
  minstd_rand gen;
  graph_type graph(small_world_iterator<minstd_rand>(gen, n, k, p),
                   small_world_iterator<minstd_rand>(n, k),
                   n);
#endif
#endif
  // boost::read_graphviz(stdin, graph);

  typedef boost::property_map<graph_type, boost::vertex_index_t>::type 
    VertexIndexMap;
  VertexIndexMap vertex_index = get(boost::vertex_index_t(), graph);

  typedef boost::heart_topology<> topology;
  topology space;

  typedef topology::point_type point;
  std::vector<point> position_vector(num_vertices(graph));
  typedef boost::iterator_property_map<std::vector<point>::iterator,
                                       VertexIndexMap, point, point&> Position;
  Position position(position_vector.begin(), vertex_index);

  boost::gursoy_atun_layout(graph, space, position);

#if 0
  std::cerr << "--------Unweighted layout--------\n";
  boost::write_graphviz(std::cout, graph, 
                        position_writer<Position, vertex_descriptor>(position),
                        boost::default_writer(),
                        graph_writer());
#endif

  boost::gursoy_atun_layout(graph, space, position,
                            weight_map(get(boost::edge_weight, graph)));

#if 0
  std::cerr << "--------Weighted layout--------\n";
  boost::write_graphviz(std::cout, graph, 
                        position_writer<Position, vertex_descriptor>(position),
                        boost::default_writer(),
                        graph_writer());
#endif
  return 0;
}
