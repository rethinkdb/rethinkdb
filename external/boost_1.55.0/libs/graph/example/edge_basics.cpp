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
#include <algorithm>
#include <boost/graph/adjacency_list.hpp>

using namespace std;
using namespace boost;


/*
  Edge Basics

  This example demonstrates the GGCL Edge interface

  There is not much to the Edge interface. Basically just two
  functions to access the source and target vertex:
  
  source(e)
  target(e)

  and one associated type for the vertex type:

  edge_traits<Edge>::vertex_type

  Sample output:

  (0,1) (0,2) (0,3) (0,4) (2,0) (2,4) (3,0) (3,1) 

 */



template <class Graph>
struct exercise_edge {
  exercise_edge(Graph& g) : G(g) {}

  typedef typename boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
  void operator()(Edge e) const
  {
    //begin
    // Get the associated vertex type out of the edge using the
    // edge_traits class
    // Use the source() and target() functions to access the vertices
    // that belong to Edge e
    Vertex src = source(e, G);
    Vertex targ = target(e, G);

    // print out the vertex id's just because 
    cout << "(" << src << "," << targ << ") ";
    //end
  }

  Graph& G;
};


int
main()
{
  typedef adjacency_list<> MyGraph;

  typedef pair<int,int> Pair;
  Pair edge_array[8] = { Pair(0,1), Pair(0,2), Pair(0,3), Pair(0,4), 
                         Pair(2,0), Pair(3,0), Pair(2,4), Pair(3,1) };

  // Construct a graph using the edge_array (passing in pointers
  // (iterators) to the beginning and end of the array), and
  // specifying the number of vertices as 5
  MyGraph G(5);
  for (int i=0; i<8; ++i)
    add_edge(edge_array[i].first, edge_array[i].second, G);

  // Use the STL for_each algorithm to "exercise" all of the edges in
  // the graph
  for_each(edges(G).first, edges(G).second, exercise_edge<MyGraph>(G));
  cout << endl;
  return 0;
}
