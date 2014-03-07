//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
//  Sample output
//  DFS parenthesis:
//  (0(2(3(4(11)4)3)2)0)

#include <boost/config.hpp>
#include <assert.h>
#include <iostream>

#include <vector>
#include <algorithm>
#include <utility>

#include "boost/graph/visitors.hpp"
#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/depth_first_search.hpp"

using namespace boost;
using namespace std;

struct open_paren : public base_visitor<open_paren> {
  typedef on_discover_vertex event_filter;
  template <class Vertex, class Graph>
  void operator()(Vertex v, Graph&) {
    std::cout << "(" << v;
  }
};
struct close_paren : public base_visitor<close_paren> {
  typedef on_finish_vertex event_filter;
  template <class Vertex, class Graph>
  void operator()(Vertex v, Graph&) {
    std::cout << v << ")";
  }
};


int 
main(int, char*[])
{

  using namespace boost;
  
  typedef adjacency_list<> Graph;
  typedef std::pair<int,int> E;
  E edge_array[] = { E(0, 2),
                E(1, 1), E(1, 3),
                E(2, 1), E(2, 3),
                E(3, 1), E(3, 4),
                E(4, 0), E(4, 1) };  
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  Graph G(5);
  for (std::size_t j = 0; j < sizeof(edge_array) / sizeof(E); ++j)
    add_edge(edge_array[j].first, edge_array[j].second, G);
#else
  Graph G(edge_array, edge_array + sizeof(edge_array)/sizeof(E), 5);
#endif

  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::vertices_size_type size_type;
  
  std::cout << "DFS parenthesis:" << std::endl;
  depth_first_search(G, visitor(make_dfs_visitor(std::make_pair(open_paren(), 
                                                        close_paren()))));
  std::cout << std::endl;
  return 0;
}

