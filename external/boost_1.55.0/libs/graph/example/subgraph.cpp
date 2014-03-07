//=======================================================================
// Copyright 2001 University of Notre Dame.
// Author: Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

/*
  Sample output:

  G0:
  0 --> 1 
  1 --> 2 3 
  2 --> 5 
  3 --> 
  4 --> 1 5 
  5 --> 3 
  0(0,1) 1(1,2) 2(1,3) 6(2,5) 3(4,1) 4(4,5) 5(5,3) 

  G1:
  2 --> 5 
  4 --> 5 
  5 --> 
  6(2,5) 4(4,5) 

  G2:
  0 --> 1 
  1 --> 
  0(0,1) 

 */

#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/subgraph.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>

int main(int,char*[])
{
  using namespace boost;
  typedef subgraph< adjacency_list<vecS, vecS, directedS,
    property<vertex_color_t, int>, property<edge_index_t, int> > > Graph;

  const int N = 6;
  Graph G0(N);
  enum { A, B, C, D, E, F};     // for conveniently refering to vertices in G0

  Graph& G1 = G0.create_subgraph();
  Graph& G2 = G0.create_subgraph();
  enum { A1, B1, C1 };          // for conveniently refering to vertices in G1
  enum { A2, B2 };              // for conveniently refering to vertices in G2

  add_vertex(C, G1); // global vertex C becomes local A1 for G1
  add_vertex(E, G1); // global vertex E becomes local B1 for G1
  add_vertex(F, G1); // global vertex F becomes local C1 for G1
  
  add_vertex(A, G2); // global vertex A becomes local A1 for G2
  add_vertex(B, G2); // global vertex B becomes local B1 for G2

  add_edge(A, B, G0);
  add_edge(B, C, G0);
  add_edge(B, D, G0);
  add_edge(E, B, G0);
  add_edge(E, F, G0);
  add_edge(F, D, G0);

  add_edge(A1, C1, G1); // (A1,C1) is subgraph G1 local indices for (C,F).

  std::cout << "G0:" << std::endl;
  print_graph(G0, get(vertex_index, G0));
  print_edges2(G0, get(vertex_index, G0), get(edge_index, G0));
  std::cout << std::endl;

  Graph::children_iterator ci, ci_end;
  int num = 1;
  for (boost::tie(ci, ci_end) = G0.children(); ci != ci_end; ++ci) {
    std::cout << "G" << num++ << ":" << std::endl;
    print_graph(*ci, get(vertex_index, *ci));
    print_edges2(*ci, get(vertex_index, *ci), get(edge_index, *ci));
    std::cout << std::endl;
  }

  return 0;
}
