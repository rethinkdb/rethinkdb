//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <vector>
#include <deque>
#include <iostream>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/adjacency_list.hpp>
int
main()
{
  using namespace boost;
  const char *tasks[] = {
    "pick up kids from school",
    "buy groceries (and snacks)",
    "get cash at ATM",
    "drop off kids at soccer practice",
    "cook dinner",
    "pick up kids from soccer",
    "eat dinner"
  };
  const int n_tasks = sizeof(tasks) / sizeof(char *);

  adjacency_list < listS, vecS, directedS > g(n_tasks);

  add_edge(0, 3, g);
  add_edge(1, 3, g);
  add_edge(1, 4, g);
  add_edge(2, 1, g);
  add_edge(3, 5, g);
  add_edge(4, 6, g);
  add_edge(5, 6, g);

  std::deque < int >topo_order;

  topological_sort(g, std::front_inserter(topo_order),
                   vertex_index_map(identity_property_map()));

  int n = 1;
  for (std::deque < int >::iterator i = topo_order.begin();
       i != topo_order.end(); ++i, ++n)
    std::cout << tasks[*i] << std::endl;

  return EXIT_SUCCESS;
}
