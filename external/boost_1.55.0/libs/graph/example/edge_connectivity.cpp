//=======================================================================
// Copyright 2000 University of Notre Dame.
// Authors: Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================


#include <boost/config.hpp>
#include <set>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/edge_connectivity.hpp>

using namespace boost;

int
main()
{
  const int N = 8;
  typedef adjacency_list<vecS, vecS, undirectedS> UndirectedGraph;
  UndirectedGraph g(N);

  add_edge(0, 1, g);
  add_edge(0, 2, g);
  add_edge(0, 3, g);
  add_edge(1, 2, g);
  add_edge(1, 3, g);
  add_edge(2, 3, g);
  add_edge(3, 4, g);
  add_edge(3, 7, g);
  add_edge(4, 5, g);
  add_edge(4, 6, g);
  add_edge(4, 7, g);
  add_edge(5, 6, g);
  add_edge(5, 7, g);
  add_edge(6, 7, g);

  typedef graph_traits<UndirectedGraph>::edge_descriptor edge_descriptor;
  typedef graph_traits<UndirectedGraph>::degree_size_type degree_size_type;
  std::vector<edge_descriptor> disconnecting_set;

  degree_size_type c = edge_connectivity(g, std::back_inserter(disconnecting_set));

  std::cout << "The edge connectivity is " << c << "." << std::endl;
  std::cout << "The disconnecting set is {";

  std::copy(disconnecting_set.begin(), disconnecting_set.end(), 
            std::ostream_iterator<edge_descriptor>(std::cout, " "));
  std::cout << "}." << std::endl;
  
  return 0;
}
