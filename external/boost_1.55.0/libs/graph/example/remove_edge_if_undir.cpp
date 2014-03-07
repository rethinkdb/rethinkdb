//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>

using namespace boost;

/*
  Sample output:

  original graph:
  0 <--> 3 3 2 
  1 <--> 3 
  2 <--> 0 3 
  3 <--> 0 0 1 2 
  1(0,3) 2(0,3) 3(1,3) 4(2,0) 5(3,2) 

  removing edges connecting 0 and 3
  0 <--> 2 
  1 <--> 3 
  2 <--> 0 3 
  3 <--> 1 2 
  3(1,3) 4(2,0) 5(3,2) 
  removing edges with weight greater than 3
  0 <--> 
  1 <--> 3 
  2 <--> 
  3 <--> 1 
  3(1,3) 
  
 */

typedef adjacency_list<vecS, vecS, undirectedS, 
  no_property, property<edge_weight_t, int> > Graph;

struct has_weight_greater_than {
  has_weight_greater_than(int w_, Graph& g_) : w(w_), g(g_) { }
  bool operator()(graph_traits<Graph>::edge_descriptor e) {
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
    property_map<Graph, edge_weight_t>::type weight = get(edge_weight, g);
    return get(weight, e) > w;
#else
    // This version of get breaks VC++
    return get(edge_weight, g, e) > w;
#endif
  }
  int w;
  Graph& g;
};

int
main()
{
  typedef std::pair<std::size_t,std::size_t> Edge;
  Edge edge_array[5] = { Edge(0, 3), Edge(0, 3),
                    Edge(1, 3),
                    Edge(2, 0),
                    Edge(3, 2) };

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  Graph g(4);
  for (std::size_t j = 0; j < 5; ++j)
    add_edge(edge_array[j].first, edge_array[j].second, g);
#else
  Graph g(edge_array, edge_array + 5, 4);
#endif
  property_map<Graph, edge_weight_t>::type 
    weight = get(edge_weight, g);

  int w = 0;
  graph_traits<Graph>::edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    weight[*ei] = ++w;

  std::cout << "original graph:" << std::endl;
  print_graph(g, get(vertex_index, g));
  print_edges2(g, get(vertex_index, g), get(edge_weight, g));
  std::cout << std::endl;

  std::cout << "removing edges connecting 0 and 3" << std::endl;
  remove_out_edge_if(vertex(0, g), incident_on(vertex(3, g), g), g);
  print_graph(g, get(vertex_index, g));
  print_edges2(g, get(vertex_index, g), get(edge_weight, g));

  std::cout << "removing edges with weight greater than 3" << std::endl;
  remove_edge_if(has_weight_greater_than(3, g), g);
  print_graph(g, get(vertex_index, g));
  print_edges2(g, get(vertex_index, g), get(edge_weight, g));

  return 0;
}
