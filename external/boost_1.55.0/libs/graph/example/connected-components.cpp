//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <iostream>
#include <vector>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/adjacency_list.hpp>

int
main()
{
  using namespace boost;
  typedef adjacency_list < vecS, vecS, undirectedS > Graph;
  typedef graph_traits < Graph >::vertex_descriptor Vertex;

  const int N = 6;
  Graph G(N);
  add_edge(0, 1, G);
  add_edge(1, 4, G);
  add_edge(4, 0, G);
  add_edge(2, 5, G);

  std::vector<int> c(num_vertices(G));
  int num = connected_components
    (G, make_iterator_property_map(c.begin(), get(vertex_index, G), c[0]));

  std::cout << std::endl;
  std::vector < int >::iterator i;
  std::cout << "Total number of components: " << num << std::endl;
  for (i = c.begin(); i != c.end(); ++i)
    std::cout << "Vertex " << i - c.begin()
      << " is in component " << *i << std::endl;
  std::cout << std::endl;
  return EXIT_SUCCESS;
}
