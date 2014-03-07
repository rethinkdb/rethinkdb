//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/copy.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp>

template <typename Graph>
struct non_zero_degree {
  non_zero_degree() { } // has to have a default constructor!

  non_zero_degree(const Graph& g) : g(&g) { }

  bool operator()(typename boost::graph_traits<Graph>::vertex_descriptor v) const
  {
    return degree(v, *g) != 0;
  }
  const Graph* g;
};

int
main()
{
  using namespace boost;
  typedef int weight_t;
  typedef adjacency_list < vecS, vecS, bidirectionalS,
    property < vertex_name_t, char > > graph_t;

  enum { a, b, c, d, e, f, g, N };
  graph_t G(N);
  property_map < graph_t, vertex_name_t >::type
    name_map = get(vertex_name, G);
  char name = 'a';
  graph_traits < graph_t >::vertex_iterator v, v_end;
  for (boost::tie(v, v_end) = vertices(G); v != v_end; ++v, ++name)
    name_map[*v] = name;

  typedef std::pair < int, int >E;
  E edges[] = { E(a, c), E(a, d), E(b, a), E(b, d), E(c, f),
    E(d, c), E(d, e), E(d, f), E(e, b), E(e, g), E(f, e), E(f, g)
  };
  for (int i = 0; i < 12; ++i)
    add_edge(edges[i].first, edges[i].second, G);

  print_graph(G, name_map);
  std::cout << std::endl;

  clear_vertex(b, G);
  clear_vertex(d, G);

  graph_t G_copy;
  copy_graph(make_filtered_graph(G, keep_all(), non_zero_degree<graph_t>(G)), G_copy);

  print_graph(G_copy, get(vertex_name, G_copy));

  return 0;
}
