//=======================================================================
// Copyright 2013 Maciej Piechotka
// Authors: Maciej Piechotka
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/edge_coloring.hpp>
#include <boost/graph/properties.hpp>

/*
  Sample output
  Colored using 5 colors
    a-d: 4
    a-f: 0
    b-c: 2
    b-e: 3
    b-g: 1
    b-j: 0
    c-d: 0
    c-e: 1
    d-f: 2
    d-i: 1
    e-g: 4
    f-g: 3
    f-h: 1
    g-h: 0
*/

int main(int, char *[])
{
  using namespace boost;
  using namespace std;
  typedef adjacency_list<vecS, vecS, undirectedS, no_property, size_t, no_property> Graph;

  typedef std::pair<std::size_t, std::size_t> Pair;
  Pair edges[14] = { Pair(0,3), //a-d
                     Pair(0,5),  //a-f
                     Pair(1,2),  //b-c
                     Pair(1,4),  //b-e
                     Pair(1,6),  //b-g
                     Pair(1,9),  //b-j
                     Pair(2,3),  //c-d
                     Pair(2,4),  //c-e
                     Pair(3,5),  //d-f
                     Pair(3,8),  //d-i
                     Pair(4,6),  //e-g
                     Pair(5,6),  //f-g
                     Pair(5,7),  //f-h
                     Pair(6,7) }; //g-h

  Graph G(10);

  for (size_t i = 0; i < sizeof(edges)/sizeof(edges[0]); i++)
    add_edge(edges[i].first, edges[i].second, G).first;

  size_t colors = edge_coloring(G, get(edge_bundle, G));

  cout << "Colored using " << colors << " colors" << endl;
  for (size_t i = 0; i < sizeof(edges)/sizeof(edges[0]); i++) {
    cout << "  " << (char)('a' + edges[i].first) << "-" << (char)('a' + edges[i].second) << ": " << G[edge(edges[i].first, edges[i].second, G).first] << endl;
  }

  return 0;
}

