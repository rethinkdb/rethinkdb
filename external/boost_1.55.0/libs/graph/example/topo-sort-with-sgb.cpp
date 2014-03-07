//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <vector>
#include <string>
#include <iostream>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/stanford_graph.hpp>

int
main()
{
  using namespace boost;
  const int n_vertices = 7;
  Graph *sgb_g = gb_new_graph(n_vertices);

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

  gb_new_arc(sgb_g->vertices + 0, sgb_g->vertices + 3, 0);
  gb_new_arc(sgb_g->vertices + 1, sgb_g->vertices + 3, 0);
  gb_new_arc(sgb_g->vertices + 1, sgb_g->vertices + 4, 0);
  gb_new_arc(sgb_g->vertices + 2, sgb_g->vertices + 1, 0);
  gb_new_arc(sgb_g->vertices + 3, sgb_g->vertices + 5, 0);
  gb_new_arc(sgb_g->vertices + 4, sgb_g->vertices + 6, 0);
  gb_new_arc(sgb_g->vertices + 5, sgb_g->vertices + 6, 0);

  typedef graph_traits < Graph * >::vertex_descriptor vertex_t;
  std::vector < vertex_t > topo_order;
  topological_sort(sgb_g, std::back_inserter(topo_order),
                   vertex_index_map(get(vertex_index, sgb_g)));
  int n = 1;
  for (std::vector < vertex_t >::reverse_iterator i = topo_order.rbegin();
       i != topo_order.rend(); ++i, ++n)
    std::cout << n << ": " << tasks[get(vertex_index, sgb_g)[*i]] << std::endl;

  gb_recycle(sgb_g);
  return EXIT_SUCCESS;
}
