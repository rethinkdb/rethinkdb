//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <vector>
#include <list>
#include <boost/graph/biconnected_components.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <iterator>
#include <iostream>

namespace boost
{
  struct edge_component_t
  {
    enum
    { num = 555 };
    typedef edge_property_tag kind;
  }
  edge_component;
}

int
main()
{
  using namespace boost;
  typedef adjacency_list < vecS, vecS, undirectedS,
    no_property, property < edge_component_t, std::size_t > >graph_t;
  typedef graph_traits < graph_t >::vertex_descriptor vertex_t;
  graph_t g(9);
  add_edge(0, 5, g);
  add_edge(0, 1, g);
  add_edge(0, 6, g);
  add_edge(1, 2, g);
  add_edge(1, 3, g);
  add_edge(1, 4, g);
  add_edge(2, 3, g);
  add_edge(4, 5, g);
  add_edge(6, 8, g);
  add_edge(6, 7, g);
  add_edge(7, 8, g);

  property_map < graph_t, edge_component_t >::type
    component = get(edge_component, g);

  std::size_t num_comps = biconnected_components(g, component);
  std::cerr << "Found " << num_comps << " biconnected components.\n";

  std::vector<vertex_t> art_points;
  articulation_points(g, std::back_inserter(art_points));
  std::cerr << "Found " << art_points.size() << " articulation points.\n";

  std::cout << "graph A {\n" << "  node[shape=\"circle\"]\n";

  for (std::size_t i = 0; i < art_points.size(); ++i) {
    std::cout << (char)(art_points[i] + 'A') 
              << " [ style=\"filled\", fillcolor=\"red\" ];" 
              << std::endl;
  }

  graph_traits < graph_t >::edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    std::cout << (char)(source(*ei, g) + 'A') << " -- " 
              << (char)(target(*ei, g) + 'A')
              << "[label=\"" << component[*ei] << "\"]\n";
  std::cout << "}\n";

  return 0;
}
