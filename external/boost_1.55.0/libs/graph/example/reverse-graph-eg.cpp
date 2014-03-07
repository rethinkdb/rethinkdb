//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>

#include <algorithm>
#include <vector>
#include <utility>
#include <iostream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/graph_utility.hpp>

int
main()
{
  using namespace boost;
  typedef adjacency_list < vecS, vecS, bidirectionalS > Graph;

  Graph G(5);
  add_edge(0, 2, G);
  add_edge(1, 1, G);
  add_edge(1, 3, G);
  add_edge(1, 4, G);
  add_edge(2, 1, G);
  add_edge(2, 3, G);
  add_edge(2, 4, G);
  add_edge(3, 1, G);
  add_edge(3, 4, G);
  add_edge(4, 0, G);
  add_edge(4, 1, G);

  std::cout << "original graph:" << std::endl;
  print_graph(G, get(vertex_index, G));


  std::cout << std::endl << "reversed graph:" << std::endl;
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300  // avoid VC++ bug...
  reverse_graph<Graph> R(G);
  print_graph(R, get(vertex_index, G));
#else
  print_graph(make_reverse_graph(G), get(vertex_index, G));
#endif

  return EXIT_SUCCESS;
}
