//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <fstream>              // for file I/O
#include <boost/graph/graphviz.hpp>     // for read/write_graphviz()
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/lexical_cast.hpp>

namespace boost {
  enum graph_color_t { graph_color = 5556 };
  BOOST_INSTALL_PROPERTY(graph, color);
}

int
main()
{
  using namespace boost;
  typedef 
    adjacency_list<vecS, vecS, directedS,
                   property<vertex_name_t, std::string>, 
                   property<edge_color_t, std::string,
                            property<edge_weight_t, int> >,
                   property<graph_color_t, std::string> >
    g_dot_type;
  g_dot_type g_dot;

  dynamic_properties dp(ignore_other_properties);
  dp.property("node_id", get(vertex_name, g_dot));
  dp.property("label", get(edge_weight, g_dot));
  dp.property("color", get(edge_color, g_dot));
  dp.property("color", ref_property_map<g_dot_type*, std::string>(get_property(g_dot, graph_color)));
  {
    std::ifstream infile("figs/ospf-graph.dot");
    read_graphviz(infile, g_dot, dp);
  }

  typedef adjacency_list < vecS, vecS, directedS, no_property,
    property < edge_weight_t, int > > Graph;
  typedef graph_traits < Graph >::vertex_descriptor vertex_descriptor;
  Graph g(num_vertices(g_dot));
  graph_traits < g_dot_type >::edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = edges(g_dot); ei != ei_end; ++ei) {
    int weight = get(edge_weight, g_dot, *ei);
    property < edge_weight_t, int >edge_property(weight);
    add_edge(source(*ei, g_dot), target(*ei, g_dot), edge_property, g);
  }

  vertex_descriptor router_six;
  graph_traits < g_dot_type >::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g_dot); vi != vi_end; ++vi)
    if ("RT6" == get(vertex_name, g_dot, *vi)) {
      router_six = *vi;
      break;
    }

  std::vector < vertex_descriptor > parent(num_vertices(g));
  // All vertices start out as there own parent
  typedef graph_traits < Graph >::vertices_size_type size_type;
  for (size_type p = 0; p < num_vertices(g); ++p)
    parent[p] = p;

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  std::vector<int> distance(num_vertices(g));
  property_map<Graph, edge_weight_t>::type weightmap = get(edge_weight, g);
  property_map<Graph, vertex_index_t>::type indexmap = get(vertex_index, g);
  dijkstra_shortest_paths
    (g, router_six, &parent[0], &distance[0], weightmap,
     indexmap, std::less<int>(), closed_plus<int>(), 
     (std::numeric_limits<int>::max)(), 0, default_dijkstra_visitor());
#else
  dijkstra_shortest_paths(g, router_six, predecessor_map(&parent[0]));
#endif

  graph_traits < g_dot_type >::edge_descriptor e;
  for (size_type i = 0; i < num_vertices(g); ++i)
    if (parent[i] != i) {
      e = edge(parent[i], i, g_dot).first;
      put(edge_color, g_dot, e, "black");
    }

  get_property(g_dot, graph_color) = "grey";
  {
    std::ofstream outfile("figs/ospf-sptree.dot");
    write_graphviz_dp(outfile, g_dot, dp);
  }

  std::ofstream rtable("routing-table.dat");
  rtable << "Dest    Next Hop    Total Cost" << std::endl;
  for (boost::tie(vi, vi_end) = vertices(g_dot); vi != vi_end; ++vi)
    if (parent[*vi] != *vi) {
      rtable << get(vertex_name, g_dot, *vi) << "    ";
      vertex_descriptor v = *vi, child;
      int path_cost = 0;
      property_map < Graph, edge_weight_t >::type
        weight_map = get(edge_weight, g);
      do {
        path_cost += get(weight_map, edge(parent[v], v, g).first);
        child = v;
        v = parent[v];
      } while (v != parent[v]);
      rtable << get(vertex_name, g_dot, child) << "     ";
      rtable << path_cost << std::endl;

    }

  return EXIT_SUCCESS;
}
