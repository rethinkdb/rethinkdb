//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/config.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>

/*

  This example demonstrates the usage of the connected_components
  algorithm on a undirected graph. The example graphs come from
  "Introduction to Algorithms", Cormen, Leiserson, and Rivest p. 87
  (though we number the vertices from zero instead of one).

  Sample output:

  Total number of components: 3
  Vertex 0 is in component 0
  Vertex 1 is in component 0
  Vertex 2 is in component 1
  Vertex 3 is in component 2
  Vertex 4 is in component 0
  Vertex 5 is in component 1

 */

using namespace std;

int main(int , char* []) 
{
  using namespace boost;
  {
    typedef adjacency_list <vecS, vecS, undirectedS> Graph;

    Graph G;
    add_edge(0, 1, G);
    add_edge(1, 4, G);
    add_edge(4, 0, G);
    add_edge(2, 5, G);
    
    std::vector<int> component(num_vertices(G));
    int num = connected_components(G, &component[0]);
    
    std::vector<int>::size_type i;
    cout << "Total number of components: " << num << endl;
    for (i = 0; i != component.size(); ++i)
      cout << "Vertex " << i <<" is in component " << component[i] << endl;
    cout << endl;
  }
  return 0;
}

