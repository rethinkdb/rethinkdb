//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <iostream>
#include <fstream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
using namespace boost;

template <typename Graph, typename VertexNameMap, typename TransDelayMap>
void
build_router_network(Graph & g, VertexNameMap name_map,
                     TransDelayMap delay_map)
{
  typename graph_traits < Graph >::vertex_descriptor a, b, c, d, e;
  a = add_vertex(g);
  name_map[a] = 'a';
  b = add_vertex(g);
  name_map[b] = 'b';
  c = add_vertex(g);
  name_map[c] = 'c';
  d = add_vertex(g);
  name_map[d] = 'd';
  e = add_vertex(g);
  name_map[e] = 'e';

  typename graph_traits<Graph>::edge_descriptor ed;
  bool inserted;

  boost::tie(ed, inserted) = add_edge(a, b, g);
  delay_map[ed] = 1.2;
  boost::tie(ed, inserted) = add_edge(a, d, g);
  delay_map[ed] = 4.5;
  boost::tie(ed, inserted) = add_edge(b, d, g);
  delay_map[ed] = 1.8;
  boost::tie(ed, inserted) = add_edge(c, a, g);
  delay_map[ed] = 2.6;
  boost::tie(ed, inserted) = add_edge(c, e, g);
  delay_map[ed] = 5.2;
  boost::tie(ed, inserted) = add_edge(d, c, g);
  delay_map[ed] = 0.4;
  boost::tie(ed, inserted) = add_edge(d, e, g);
  delay_map[ed] = 3.3;
}


template <typename VertexNameMap>
class bfs_name_printer : public default_bfs_visitor {
                         // inherit default (empty) event point actions
public:
  bfs_name_printer(VertexNameMap n_map) : m_name_map(n_map) {
  }
  template <typename Vertex, typename Graph>
  void discover_vertex(Vertex u, const Graph &) const
  {
    std::cout << get(m_name_map, u) << ' ';
  }
private:
  VertexNameMap m_name_map;
};

struct VP {
  char name;
};

struct EP {
  double weight;
};


int
main()
{
  typedef adjacency_list < listS, vecS, directedS, VP, EP> graph_t;
  graph_t g;

  property_map<graph_t, char VP::*>::type name_map = get(&VP::name, g);
  property_map<graph_t, double EP::*>::type delay_map = get(&EP::weight, g);

  build_router_network(g, name_map, delay_map);

  typedef property_map<graph_t, char VP::*>::type VertexNameMap;
  graph_traits<graph_t>::vertex_descriptor a = *vertices(g).first;
  bfs_name_printer<VertexNameMap> vis(name_map);
  std::cout << "BFS vertex discover order: ";
  breadth_first_search(g, a, visitor(vis));
  std::cout << std::endl;

}
