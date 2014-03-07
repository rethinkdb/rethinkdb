//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/config.hpp>

#include <algorithm>
#include <vector>
#include <utility>
#include <iostream>

#include <boost/graph/visitors.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/neighbor_bfs.hpp>
#include <boost/property_map/property_map.hpp>

/*
  
  Sample Output:

  0 --> 2 
  1 --> 1 3 4 
  2 --> 1 3 4 
  3 --> 1 4 
  4 --> 0 1 
  distances: 0 2 1 2 1 
  parent[0] = 0
  parent[1] = 2
  parent[2] = 0
  parent[3] = 2
  parent[4] = 0

*/

using namespace boost;

template <class ParentDecorator>
struct print_parent {
  print_parent(const ParentDecorator& p_) : p(p_) { }
  template <class Vertex>
  void operator()(const Vertex& v) const {
    std::cout << "parent[" << v << "] = " <<  p[v]  << std::endl;
  }
  ParentDecorator p;
};

template <class DistanceMap, class PredecessorMap, class ColorMap>
class distance_and_pred_visitor : public neighbor_bfs_visitor<>
{
  typedef typename property_traits<ColorMap>::value_type ColorValue;
  typedef color_traits<ColorValue> Color;
public:
  distance_and_pred_visitor(DistanceMap d, PredecessorMap p, ColorMap c)
    : m_distance(d), m_predecessor(p), m_color(c) { }

  template <class Edge, class Graph>
  void tree_out_edge(Edge e, const Graph& g) const
  {
    typename graph_traits<Graph>::vertex_descriptor 
      u = source(e, g), v = target(e, g);
    put(m_distance, v, get(m_distance, u) + 1);
    put(m_predecessor, v, u);
  }
  template <class Edge, class Graph>
  void tree_in_edge(Edge e, const Graph& g) const
  {
    typename graph_traits<Graph>::vertex_descriptor 
      u = source(e, g), v = target(e, g);
    put(m_distance, u, get(m_distance, v) + 1);
    put(m_predecessor, u, v);
  }

  DistanceMap m_distance;
  PredecessorMap m_predecessor;
  ColorMap m_color;
};

int main(int , char* []) 
{
  typedef adjacency_list< 
    mapS, vecS, bidirectionalS,
    property<vertex_color_t, default_color_type>
  > Graph;

  typedef property_map<Graph, vertex_color_t>::type
    ColorMap;
  
  Graph G(5);
  add_edge(0, 2, G);
  add_edge(1, 1, G);
  add_edge(1, 3, G);
  add_edge(1, 4, G);
  add_edge(2, 1, G);
  add_edge(2, 3, G);
  add_edge(2, 4, G);
  add_edge(3, 1, G);
  add_edge(3, 4, G);
  add_edge(4, 0, G);
  add_edge(4, 1, G);

  typedef Graph::vertex_descriptor Vertex;

  // Array to store predecessor (parent) of each vertex. This will be
  // used as a Decorator (actually, its iterator will be).
  std::vector<Vertex> p(num_vertices(G));
  // VC++ version of std::vector has no ::pointer, so
  // I use ::value_type* instead.
  typedef std::vector<Vertex>::value_type* Piter;

  // Array to store distances from the source to each vertex .  We use
  // a built-in array here just for variety. This will also be used as
  // a Decorator.  
  typedef graph_traits<Graph>::vertices_size_type size_type;
  size_type d[5];
  std::fill_n(d, 5, 0);

  // The source vertex
  Vertex s = *(vertices(G).first);
  p[s] = s;
  distance_and_pred_visitor<size_type*, Vertex*, ColorMap> 
    vis(d, &p[0], get(vertex_color, G));
  neighbor_breadth_first_search
    (G, s, visitor(vis).
     color_map(get(vertex_color, G)));

  print_graph(G);

  if (num_vertices(G) < 11) {
    std::cout << "distances: ";
#ifdef BOOST_OLD_STREAM_ITERATORS
    std::copy(d, d + 5, std::ostream_iterator<int, char>(std::cout, " "));
#else
    std::copy(d, d + 5, std::ostream_iterator<int>(std::cout, " "));
#endif
    std::cout << std::endl;

    std::for_each(vertices(G).first, vertices(G).second, 
                  print_parent<Piter>(&p[0]));
  }

  return 0;
}
