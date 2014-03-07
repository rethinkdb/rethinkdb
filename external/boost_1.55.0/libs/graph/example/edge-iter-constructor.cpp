//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <fstream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>

using namespace boost;

template < typename T >
  std::istream & operator >> (std::istream & in, std::pair < T, T > &p) {
  in >> p.first >> p.second;
  return in;
}


int
main()
{
  typedef adjacency_list <
    listS,                     // Store out-edges of each vertex in a std::list
    vecS,                      // Store vertex set in a std::vector
    directedS                  // The graph is directed
    > graph_type;

  std::ifstream file_in("makefile-dependencies.dat");
  typedef graph_traits < graph_type >::vertices_size_type size_type;
  size_type n_vertices;
  file_in >> n_vertices;        // read in number of vertices

  graph_type
  g(n_vertices);                // create graph with n vertices

  // Read in edges
  graph_traits < graph_type >::vertices_size_type u, v;
  while (file_in >> u)
    if (file_in >> v)
      add_edge(u, v, g);
    else
      break;

  assert(num_vertices(g) == 15);
  assert(num_edges(g) == 19);
  return 0;
}
