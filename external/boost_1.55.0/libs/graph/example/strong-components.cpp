//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <vector>
#include <iostream>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/adjacency_list.hpp>

int
main()
{
  using namespace boost;
  typedef adjacency_list < vecS, vecS, directedS > Graph;
  const int N = 6;
  Graph G(N);
  add_edge(0, 1, G);
  add_edge(1, 1, G);
  add_edge(1, 3, G);
  add_edge(1, 4, G);
  add_edge(3, 4, G);
  add_edge(3, 0, G);
  add_edge(4, 3, G);
  add_edge(5, 2, G);

  std::vector<int> c(N);
  int num = strong_components
    (G, make_iterator_property_map(c.begin(), get(vertex_index, G), c[0]));

  std::cout << "Total number of components: " << num << std::endl;
  std::vector < int >::iterator i;
  for (i = c.begin(); i != c.end(); ++i)
    std::cout << "Vertex " << i - c.begin()
      << " is in component " << *i << std::endl;
  return EXIT_SUCCESS;
}
