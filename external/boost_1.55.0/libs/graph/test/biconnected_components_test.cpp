// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#include <boost/graph/biconnected_components.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/test/minimal.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/random.hpp>
#include <boost/random/linear_congruential.hpp>
#include <fstream>

using namespace boost;

struct EdgeProperty
{
  std::size_t component;
};

static bool any_errors = false;

template<typename Graph, typename Vertex>
void 
check_articulation_points(const Graph& g, std::vector<Vertex> art_points)
{
  std::vector<int> components(num_vertices(g));
  int basic_comps = 
    connected_components(g, 
                         make_iterator_property_map(components.begin(), 
                                                    get(vertex_index, g),
                                                    int()));

  std::vector<Vertex> art_points_check;
  
  typename graph_traits<Graph>::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    Graph g_copy(g);
    Vertex victim = vertex(get(vertex_index, g, *vi), g_copy);
    clear_vertex(victim, g_copy);
    remove_vertex(victim, g_copy);

    int copy_comps = 
      connected_components
        (g_copy, 
         make_iterator_property_map(components.begin(), 
                                    get(vertex_index, g_copy),
                                    int()));

    if (copy_comps > basic_comps)
      art_points_check.push_back(*vi);
  }

  std::sort(art_points.begin(), art_points.end());
  std::sort(art_points_check.begin(), art_points_check.end());

  BOOST_CHECK(art_points == art_points_check);
  if (art_points != art_points_check) {
    std::cerr << "ERROR!" << std::endl;
    std::cerr << "\tComputed: ";
    std::size_t i;
    for (i = 0; i < art_points.size(); ++i) 
      std::cout << art_points[i] << ' ';
    std::cout << std::endl << "\tExpected: ";
    for (i = 0; i < art_points_check.size(); ++i)
      std::cout << art_points_check[i] << ' ';
    std::cout << std::endl;
    any_errors = true;
  } else std::cout << "OK." << std::endl;
}

typedef adjacency_list<listS, vecS, undirectedS,
                       no_property, EdgeProperty> Graph;
typedef graph_traits<Graph>::vertex_descriptor Vertex;

bool test_graph(Graph& g) { // Returns false on failure
  std::vector<Vertex> art_points;

  std::cout << "Computing biconnected components & articulation points... ";
  std::cout.flush();

  std::size_t num_comps = 
    biconnected_components(g, 
                           get(&EdgeProperty::component, g),
                           std::back_inserter(art_points)).first;
  
  std::cout << "done.\n\t" << num_comps << " biconnected components.\n"
            << "\t" << art_points.size() << " articulation points.\n"
            << "\tTesting articulation points...";
  std::cout.flush();

  check_articulation_points(g, art_points);

  if (any_errors) {
    std::ofstream out("biconnected_components_test_failed.dot");

    out << "graph A {\n" << "  node[shape=\"circle\"]\n";

    for (std::size_t i = 0; i < art_points.size(); ++i) {
      out << art_points[i] << " [ style=\"filled\" ];" << std::endl;
    }
    
    graph_traits<Graph>::edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
      out << source(*ei, g) << " -- " << target(*ei, g)
          << "[label=\"" << g[*ei].component << "\"]\n";
    out << "}\n";
  }

  return any_errors;
}
  
int test_main(int argc, char* argv[])
{
  std::size_t n = 100;
  std::size_t m = 500;
  std::size_t seed = 1;

  if (argc > 1) n = lexical_cast<std::size_t>(argv[1]);
  if (argc > 2) m = lexical_cast<std::size_t>(argv[2]);
  if (argc > 3) seed = lexical_cast<std::size_t>(argv[3]);

  {
    Graph g(n);
    minstd_rand gen(seed);
    generate_random_graph(g, n, m, gen);
    if (test_graph(g)) return 1;
  }

  {
    Graph g(4);
    add_edge(2, 3, g);
    add_edge(0, 3, g);
    add_edge(0, 2, g);
    add_edge(1, 0, g);
    if (test_graph(g)) return 1;
  }

  return 0;
}
