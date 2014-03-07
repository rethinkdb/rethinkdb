//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <iostream>
#include <string>
#include <boost/graph/adjacency_list.hpp>
int
main()
{
  using namespace boost;
  typedef adjacency_list < listS, listS, directedS,
    property < vertex_name_t, std::string > >graph_t;
  graph_t g;
  graph_traits < graph_t >::vertex_descriptor u = add_vertex(g);
  property_map < graph_t, vertex_name_t >::type
    name_map = get(vertex_name, g);
  name_map[u] = "Joe";
  std::cout << name_map[u] << std::endl;
  return EXIT_SUCCESS;
}
