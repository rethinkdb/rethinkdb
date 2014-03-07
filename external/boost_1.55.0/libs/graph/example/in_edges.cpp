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
#include <utility>

#include <boost/graph/adjacency_list.hpp>

/*
  Sample Output

  0 <-- 
  1 <-- 0  
  2 <-- 1  
  3 <-- 1  
  4 <-- 2  3  

 */

int main(int , char* [])
{
  using namespace boost;
  using namespace std;
  using namespace boost;

  typedef adjacency_list<listS,vecS,bidirectionalS> Graph;
  const int num_vertices = 5;
  Graph g(num_vertices);

  add_edge(0, 1, g);
  add_edge(1, 2, g);
  add_edge(1, 3, g);
  add_edge(2, 4, g);
  add_edge(3, 4, g);

  boost::graph_traits<Graph>::vertex_iterator i, end;
  boost::graph_traits<Graph>::in_edge_iterator ei, edge_end;

  for(boost::tie(i,end) = vertices(g); i != end; ++i) {
    cout << *i << " <-- ";
    for (boost::tie(ei,edge_end) = in_edges(*i, g); ei != edge_end; ++ei)
      cout << source(*ei, g) << "  ";
    cout << endl;
  }
  return 0;
}
