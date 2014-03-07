//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>


int main(int argc, char** argv)
{

  // This program illustrates a simple use of boyer_myrvold_planar_embedding
  // as a simple yes/no test for planarity.

  using namespace boost;

  typedef adjacency_list<vecS,
                         vecS,
                         undirectedS,
                         property<vertex_index_t, int>
                         > graph;

  graph K_4(4);
  add_edge(0, 1, K_4);
  add_edge(0, 2, K_4);
  add_edge(0, 3, K_4);
  add_edge(1, 2, K_4);
  add_edge(1, 3, K_4);
  add_edge(2, 3, K_4);

  if (boyer_myrvold_planarity_test(K_4))
    std::cout << "K_4 is planar." << std::endl;
  else
    std::cout << "ERROR! K_4 should have been recognized as planar!" 
          << std::endl;

  graph K_5(5);
  add_edge(0, 1, K_5);
  add_edge(0, 2, K_5);
  add_edge(0, 3, K_5);
  add_edge(0, 4, K_5);
  add_edge(1, 2, K_5);
  add_edge(1, 3, K_5);
  add_edge(1, 4, K_5);
  add_edge(2, 3, K_5);
  add_edge(2, 4, K_5);

  // We've almost created a K_5 - it's missing one edge - so it should still
  // be planar at this point.

  if (boyer_myrvold_planarity_test(K_5))
    std::cout << "K_5 (minus an edge) is planar." << std::endl;
  else
    std::cout << "ERROR! K_5 with one edge missing should"
              << " have been recognized as planar!" << std::endl;

  // Now add the final edge...
  add_edge(3, 4, K_5);
  
  if (boyer_myrvold_planarity_test(K_5))
    std::cout << "ERROR! K_5 was recognized as planar!" << std::endl;
  else
    std::cout << "K_5 is not planar." << std::endl;

  return 0;
}
