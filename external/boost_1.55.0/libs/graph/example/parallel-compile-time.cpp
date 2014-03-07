//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/property_iter_range.hpp>
#include <boost/graph/depth_first_search.hpp>   // for default_dfs_visitor

namespace std
{
  template < typename T >
    std::istream & operator >> (std::istream & in, std::pair < T, T > &p)
  {
    in >> p.first >> p.second;
    return in;
  }
}

namespace boost
{
  enum vertex_compile_cost_t { vertex_compile_cost };
  BOOST_INSTALL_PROPERTY(vertex, compile_cost);
}

using namespace boost;

typedef adjacency_list < listS, // Store out-edges of each vertex in a std::list
  listS,                        // Store vertex set in a std::list
  directedS,                    // The file dependency graph is directed
  // vertex properties
  property < vertex_name_t, std::string, 
  property < vertex_compile_cost_t, float,
  property < vertex_distance_t, float, 
  property < vertex_color_t, default_color_type > > > >,
  // an edge property
  property < edge_weight_t, float > >
  file_dep_graph2;

typedef graph_traits<file_dep_graph2>::vertex_descriptor vertex_t;
typedef graph_traits<file_dep_graph2>::edge_descriptor edge_t;


template < typename Graph, typename ColorMap, typename Visitor > void
dfs_v2(const Graph & g,
       typename graph_traits < Graph >::vertex_descriptor u,
       ColorMap color, Visitor vis)
{
  typedef typename property_traits < ColorMap >::value_type color_type;
  typedef color_traits < color_type > ColorT;
  color[u] = ColorT::gray();
  vis.discover_vertex(u, g);
  typename graph_traits < Graph >::out_edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei)
    if (color[target(*ei, g)] == ColorT::white()) {
      vis.tree_edge(*ei, g);
      dfs_v2(g, target(*ei, g), color, vis);
    } else if (color[target(*ei, g)] == ColorT::gray())
      vis.back_edge(*ei, g);
    else
      vis.forward_or_cross_edge(*ei, g);
  color[u] = ColorT::black();
  vis.finish_vertex(u, g);
}

template < typename Graph, typename Visitor, typename ColorMap > void
generic_dfs_v2(const Graph & g, Visitor vis, ColorMap color)
{
  typedef typename property_traits <ColorMap >::value_type ColorValue;
  typedef color_traits < ColorValue >  ColorT;
  typename graph_traits < Graph >::vertex_iterator  vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    color[*vi] = ColorT::white();
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    if (color[*vi] == ColorT::white())
      dfs_v2(g, *vi, color, vis);
}


template < typename OutputIterator > 
struct topo_visitor: public default_dfs_visitor
{
  topo_visitor(OutputIterator & order):
  topo_order(order)
  {
  }
  template < typename Graph > void
  finish_vertex(typename graph_traits < Graph >::vertex_descriptor u,
                const Graph &)
  {
    *topo_order++ = u;
  }
  OutputIterator & topo_order;
};

template < typename Graph, typename OutputIterator, typename ColorMap > void
topo_sort(const Graph & g, OutputIterator topo_order, ColorMap color)
{
  topo_visitor < OutputIterator > vis(topo_order);
  generic_dfs_v2(g, vis, color);
}


typedef property_map < file_dep_graph2, vertex_name_t >::type name_map_t;
typedef property_map < file_dep_graph2, vertex_compile_cost_t >::type
  compile_cost_map_t;
typedef property_map < file_dep_graph2, vertex_distance_t >::type distance_map_t;
typedef property_map < file_dep_graph2, vertex_color_t >::type color_map_t;


int
main()
{
  std::ifstream file_in("makefile-dependencies.dat");
  typedef graph_traits < file_dep_graph2 >::vertices_size_type size_type;
  size_type n_vertices;
  file_in >> n_vertices;        // read in number of vertices
  std::istream_iterator < std::pair < size_type,
    size_type > >input_begin(file_in), input_end;
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ can't handle the iterator constructor
  file_dep_graph2 g;
  typedef graph_traits<file_dep_graph2 >::vertex_descriptor vertex_t;
  std::vector<vertex_t> id2vertex;
  for (std::size_t v = 0; v < n_vertices; ++v)
    id2vertex.push_back(add_vertex(g));
  while (input_begin != input_end) {
    size_type i, j;
    boost::tie(i, j) = *input_begin++;
    add_edge(id2vertex[i], id2vertex[j], g);
  }
#else
  file_dep_graph2 g(input_begin, input_end, n_vertices);
#endif

  name_map_t
    name_map =
    get(vertex_name, g);
  compile_cost_map_t
    compile_cost_map =
    get(vertex_compile_cost, g);
  distance_map_t
    distance_map =
    get(vertex_distance, g);
  color_map_t
    color_map =
    get(vertex_color, g);

  {
    std::ifstream name_in("makefile-target-names.dat");
    std::ifstream compile_cost_in("target-compile-costs.dat");
    graph_traits < file_dep_graph2 >::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
      name_in >> name_map[*vi];
      compile_cost_in >> compile_cost_map[*vi];
    }

  }
  std::vector < vertex_t > topo_order(num_vertices(g));
  topo_sort(g, topo_order.rbegin(), color_map);

  graph_traits < file_dep_graph2 >::vertex_iterator i, i_end;
  graph_traits < file_dep_graph2 >::adjacency_iterator vi, vi_end;

  // find source vertices with zero in-degree by marking all vertices with incoming edges
  for (boost::tie(i, i_end) = vertices(g); i != i_end; ++i)
    color_map[*i] = white_color;
  for (boost::tie(i, i_end) = vertices(g); i != i_end; ++i)
    for (boost::tie(vi, vi_end) = adjacent_vertices(*i, g); vi != vi_end; ++vi)
      color_map[*vi] = black_color;

  // initialize distances to zero, or for source vertices, to the compile cost
  for (boost::tie(i, i_end) = vertices(g); i != i_end; ++i)
    if (color_map[*i] == white_color)
      distance_map[*i] = compile_cost_map[*i];
    else
      distance_map[*i] = 0;

  std::vector < vertex_t >::iterator ui;
  for (ui = topo_order.begin(); ui != topo_order.end(); ++ui) {
    vertex_t
      u = *
      ui;
    for (boost::tie(vi, vi_end) = adjacent_vertices(u, g); vi != vi_end; ++vi)
      if (distance_map[*vi] < distance_map[u] + compile_cost_map[*vi])
        distance_map[*vi] = distance_map[u] + compile_cost_map[*vi];
  }

  graph_property_iter_range < file_dep_graph2,
    vertex_distance_t >::iterator ci, ci_end;
  boost::tie(ci, ci_end) = get_property_iter_range(g, vertex_distance);
  std::cout << "total (parallel) compile time: "
    << *std::max_element(ci, ci_end) << std::endl;

  return 0;
}
