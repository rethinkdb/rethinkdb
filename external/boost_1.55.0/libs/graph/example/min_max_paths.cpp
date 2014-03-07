//=======================================================================
// Copyright 1997-2001 University of Notre Dame.
// Authors: Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#error "This example appears to be incorrect; it uses edge weights that are smaller than 0 using the comparison operator passed to Dijkstra's algorithm, which is not allowed."

#include <boost/config.hpp>
#include <iostream>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/transpose_graph.hpp>

/* Output:

  distances from start vertex:
  distance(a) = 0
  distance(b) = 3
  distance(c) = 1
  distance(d) = 3
  distance(e) = 3

  min-max paths tree
  a --> c 
  b --> 
  c --> d 
  d --> e 
  e --> b 

*/

int 
main(int , char* [])
{
  using namespace boost;

  typedef adjacency_list<listS, vecS, directedS, 
    no_property, property<edge_weight_t, int> > Graph;
  typedef graph_traits<Graph>::vertex_descriptor Vertex;

  typedef std::pair<int,int> E;

  const char name[] = "abcdef";

  const int num_nodes = 6;
  E edges[] = { E(0,2), E(1,1), E(1,3), E(1,4), E(2,1), E(2,3), 
                E(3,4), E(4,0), E(4,1) };
  int weights[] = { 1, 2, 1, 2, 7, 3, 1, 1, 1};
  const int n_edges = sizeof(edges)/sizeof(E);
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ can't handle iterator constructors
  Graph G(num_nodes);
  property_map<Graph, edge_weight_t>::type weightmap = get(edge_weight, G);
  for (std::size_t j = 0; j < sizeof(edges) / sizeof(E); ++j) {
    graph_traits<Graph>::edge_descriptor e; bool inserted;
    boost::tie(e, inserted) = add_edge(edges[j].first, edges[j].second, G);
    weightmap[e] = weights[j];
  }
#else
  Graph G(edges, edges + n_edges, weights, num_nodes);
  property_map<Graph, edge_weight_t>::type weightmap = get(edge_weight, G);
#endif

  std::vector<Vertex> p(num_vertices(G));
  std::vector<int> d(num_vertices(G));

  Vertex s = *(vertices(G).first);

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  dijkstra_shortest_paths
    (G, s, &p[0], &d[0], weightmap, get(vertex_index, G),
     std::greater<int>(), closed_plus<int>(), (std::numeric_limits<int>::max)(), 0,
     default_dijkstra_visitor());
#else
  dijkstra_shortest_paths
    (G, s, distance_map(&d[0]).
     predecessor_map(&p[0]).
     distance_compare(std::greater<int>()));
#endif

  std::cout << "distances from start vertex:" << std::endl;
  graph_traits<Graph>::vertex_iterator vi, vend;
  for(boost::tie(vi,vend) = vertices(G); vi != vend; ++vi)
    std::cout << "distance(" << name[*vi] << ") = " << d[*vi] << std::endl;
  std::cout << std::endl;

  std::cout << "min-max paths tree" << std::endl;
  adjacency_list<> tree(num_nodes);
  
  for(boost::tie(vi,vend) = vertices(G); vi != vend; ++vi)
    if (*vi != p[*vi])
      add_edge(p[*vi], *vi, tree);

  print_graph(tree, name);

  return 0;
}
