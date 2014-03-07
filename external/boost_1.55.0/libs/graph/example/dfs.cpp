//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <assert.h>
#include <iostream>

#include <vector>
#include <algorithm>
#include <utility>


#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/visitors.hpp>

/*
  This calculates the discover finishing time.

  Sample Output

  Tree edge: 0 --> 2
  Tree edge: 2 --> 1
  Back edge: 1 --> 1
  Finish edge: 1 --> 1
  Tree edge: 1 --> 3
  Back edge: 3 --> 1
  Finish edge: 3 --> 1
  Tree edge: 3 --> 4
  Back edge: 4 --> 0
  Finish edge: 4 --> 0
  Back edge: 4 --> 1
  Finish edge: 4 --> 1
  Forward or cross edge: 2 --> 3
  Finish edge: 2 --> 3
  Finish edge: 0 --> 2
  1 10
  3 8
  2 9
  4 7
  5 6

 */

using namespace boost;
using namespace std;


template <class VisitorList>
struct edge_categorizer : public dfs_visitor<VisitorList> {
  typedef dfs_visitor<VisitorList> Base;

  edge_categorizer(const VisitorList& v = null_visitor()) : Base(v) { }

  template <class Edge, class Graph>
  void tree_edge(Edge e, Graph& G) {
    cout << "Tree edge: " << source(e, G) <<
      " --> " <<  target(e, G) << endl;
    Base::tree_edge(e, G);
  }
  template <class Edge, class Graph>
  void back_edge(Edge e, Graph& G) {
    cout << "Back edge: " << source(e, G)
         << " --> " <<  target(e, G) << endl;
    Base::back_edge(e, G);
  }
  template <class Edge, class Graph>
  void forward_or_cross_edge(Edge e, Graph& G) {
    cout << "Forward or cross edge: " << source(e, G)
         << " --> " <<  target(e, G) << endl;
    Base::forward_or_cross_edge(e, G);
  }
  template <class Edge, class Graph> 
  void finish_edge(Edge e, Graph& G) { 
    cout << "Finish edge: " << source(e, G) << 
      " --> " <<  target(e, G) << endl; 
    Base::finish_edge(e, G); 
  } 
};
template <class VisitorList>
edge_categorizer<VisitorList>
categorize_edges(const VisitorList& v) {
  return edge_categorizer<VisitorList>(v);
}

int 
main(int , char* [])
{

  using namespace boost;
  
  typedef adjacency_list<> Graph;
  
  Graph G(5);
  add_edge(0, 2, G);
  add_edge(1, 1, G);
  add_edge(1, 3, G);
  add_edge(2, 1, G);
  add_edge(2, 3, G);
  add_edge(3, 1, G);
  add_edge(3, 4, G);
  add_edge(4, 0, G);
  add_edge(4, 1, G);

  typedef graph_traits<Graph>::vertex_descriptor Vertex;
  typedef graph_traits<Graph>::vertices_size_type size_type;

  std::vector<size_type> d(num_vertices(G));  
  std::vector<size_type> f(num_vertices(G));
  int t = 0;
  depth_first_search(G, visitor(categorize_edges(
                     make_pair(stamp_times(&d[0], t, on_discover_vertex()),
                               stamp_times(&f[0], t, on_finish_vertex())))));

  std::vector<size_type>::iterator i, j;
  for (i = d.begin(), j = f.begin(); i != d.end(); ++i, ++j)
    cout << *i << " " << *j << endl;

  return 0;
}

