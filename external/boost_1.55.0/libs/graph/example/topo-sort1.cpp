//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <deque>                // to store the vertex ordering
#include <vector>
#include <list>
#include <iostream>
#include <boost/graph/vector_as_graph.hpp>
#include <boost/graph/topological_sort.hpp>

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

  std::vector < std::list < int > > g(n_tasks);
  g[0].push_back(3);
  g[1].push_back(3);
  g[1].push_back(4);
  g[2].push_back(1);
  g[3].push_back(5);
  g[4].push_back(6);
  g[5].push_back(6);

  std::deque < int >topo_order;

  topological_sort(g, std::front_inserter(topo_order),
                   vertex_index_map(identity_property_map()));

  int n = 1;
  for (std::deque < int >::iterator i = topo_order.begin();
       i != topo_order.end(); ++i, ++n)
    std::cout << tasks[*i] << std::endl;

  return EXIT_SUCCESS;
}
