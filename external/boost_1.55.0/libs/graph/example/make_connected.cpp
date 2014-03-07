//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <vector>

#include <boost/graph/connected_components.hpp>
#include <boost/graph/make_connected.hpp>



using namespace boost;

int main(int argc, char** argv)
{

  typedef adjacency_list
    < vecS,
      vecS,
      undirectedS,
      property<vertex_index_t, int>
    > 
    graph;

  graph g(11);
  add_edge(0,1,g);
  add_edge(2,3,g);
  add_edge(3,4,g);
  add_edge(5,6,g);
  add_edge(6,7,g);
  add_edge(8,9,g);
  add_edge(9,10,g);
  add_edge(10,8,g);

  std::vector< graph_traits<graph>::vertices_size_type > 
    component(num_vertices(g));
  
  std::cout << "Before calling make_connected, the graph has "
            << connected_components(g, &component[0])
            << " connected components" << std::endl;

  make_connected(g);

  std::cout << "After calling make_connected, the graph has "
            << connected_components(g, &component[0])
            << " connected components" << std::endl;

  return 0;
}
