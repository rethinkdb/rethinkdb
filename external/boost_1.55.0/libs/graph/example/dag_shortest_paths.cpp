//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/dag_shortest_paths.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <iostream>

// Example from Introduction to Algorithms by Cormen, et all p.537.

// Sample output:
//  r: inifinity
//  s: 0
//  t: 2
//  u: 6
//  v: 5
//  x: 3

int main()
{
  using namespace boost;
  typedef adjacency_list<vecS, vecS, directedS, 
    property<vertex_distance_t, int>, property<edge_weight_t, int> > graph_t;
  graph_t g(6);
  enum verts { r, s, t, u, v, x };
  char name[] = "rstuvx";
  add_edge(r, s, 5, g);
  add_edge(r, t, 3, g);
  add_edge(s, t, 2, g);
  add_edge(s, u, 6, g);
  add_edge(t, u, 7, g);
  add_edge(t, v, 4, g);
  add_edge(t, x, 2, g);
  add_edge(u, v, -1, g);
  add_edge(u, x, 1, g);
  add_edge(v, x, -2, g);

  property_map<graph_t, vertex_distance_t>::type
    d_map = get(vertex_distance, g);

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ has trouble with the named-parameter mechanism, so
  // we make a direct call to the underlying implementation function.
  std::vector<default_color_type> color(num_vertices(g));
  std::vector<std::size_t> pred(num_vertices(g));
  default_dijkstra_visitor vis;
  std::less<int> compare;
  closed_plus<int> combine;
  property_map<graph_t, edge_weight_t>::type w_map = get(edge_weight, g);
  dag_shortest_paths(g, s, d_map, w_map, &color[0], &pred[0], 
     vis, compare, combine, (std::numeric_limits<int>::max)(), 0);
#else
  dag_shortest_paths(g, s, distance_map(d_map));
#endif

  graph_traits<graph_t>::vertex_iterator vi , vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    if (d_map[*vi] == (std::numeric_limits<int>::max)())
      std::cout << name[*vi] << ": inifinity\n";
    else
      std::cout << name[*vi] << ": " << d_map[*vi] << '\n';
  return 0;
}
