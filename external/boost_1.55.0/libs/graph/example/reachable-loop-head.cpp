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
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>

int
main(int argc, char *argv[])
{
  if (argc < 3) {
    std::cerr << "usage: reachable-loop-head.exe <in-file> <out-file>"
      << std::endl;
    return -1;
  }
  using namespace boost;
  GraphvizDigraph g;
  read_graphviz(argv[1], g);
  graph_traits < GraphvizDigraph >::vertex_descriptor loop_head = 1;
  typedef color_traits < default_color_type > Color;

  std::vector < default_color_type >
    reachable_from_head(num_vertices(g), Color::white());
  default_color_type c;
  depth_first_visit(g, loop_head, default_dfs_visitor(),
                    make_iterator_property_map(reachable_from_head.begin(),
                                               get(vertex_index, g), c));

  property_map<GraphvizDigraph, vertex_attribute_t>::type
    vattr_map = get(vertex_attribute, g);

  graph_traits < GraphvizDigraph >::vertex_iterator i, i_end;
  for (boost::tie(i, i_end) = vertices(g); i != i_end; ++i)
    if (reachable_from_head[*i] != Color::white()) {
      vattr_map[*i]["color"] = "gray";
      vattr_map[*i]["style"] = "filled";
    }

  std::ofstream loops_out(argv[2]);
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ has trouble with the get_property() functions
  loops_out << "digraph G  {\n"
            << "size=\"3,3\"\n"
            << "ratio=\"fill\"\n"
            << "shape=\"box\"\n";
  graph_traits<GraphvizDigraph>::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    loops_out << *vi << "[";
    for (std::map<std::string,std::string>::iterator ai = vattr_map[*vi].begin();
         ai != vattr_map[*vi].end(); ++ai) {
      loops_out << ai->first << "=" << ai->second;
      if (next(ai) != vattr_map[*vi].end())
        loops_out << ", ";
    }
    loops_out<< "]";
  }
  property_map<GraphvizDigraph, edge_attribute_t>::type
    eattr_map = get(edge_attribute, g);
  graph_traits<GraphvizDigraph>::edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
    loops_out << source(*ei, g) << " -> " << target(*ei, g) << "[";
    std::map<std::string,std::string>& attr_map = eattr_map[*ei];
    for (std::map<std::string,std::string>::iterator eai = attr_map.begin();
         eai != attr_map.end(); ++eai) {
      loops_out << eai->first << "=" << eai->second;
      if (next(eai) != attr_map.end())
        loops_out << ", ";
    }
    loops_out<< "]";
  }
  loops_out << "}\n";
#else
  get_property(g, graph_graph_attribute)["size"] = "3,3";
  get_property(g, graph_graph_attribute)["ratio"] = "fill";
  get_property(g, graph_vertex_attribute)["shape"] = "box";

  write_graphviz(loops_out, g,
                 make_vertex_attributes_writer(g),
                 make_edge_attributes_writer(g),
                 make_graph_attributes_writer(g));
#endif


  return EXIT_SUCCESS;
}
