//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <fstream>
#include <map>
#include <string>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/graphviz.hpp>

int
main()
{
  using namespace boost;
  GraphvizDigraph g;
  read_graphviz("figs/scc.dot", g);

  typedef graph_traits < GraphvizDigraph >::vertex_descriptor vertex_t;
  std::map < vertex_t, int >component;

  strong_components(g, make_assoc_property_map(component));

  property_map < GraphvizDigraph, vertex_attribute_t >::type
    vertex_attr_map = get(vertex_attribute, g);
  std::string color[] = {
  "white", "gray", "black", "lightgray"};
  graph_traits < GraphvizDigraph >::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    vertex_attr_map[*vi]["color"] = color[component[*vi]];
    vertex_attr_map[*vi]["style"] = "filled";
    if (vertex_attr_map[*vi]["color"] == "black")
      vertex_attr_map[*vi]["fontcolor"] = "white";
  }
  write_graphviz("figs/scc-out.dot", g);

  return EXIT_SUCCESS;
}
