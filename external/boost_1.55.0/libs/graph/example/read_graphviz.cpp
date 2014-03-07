// Copyright 2006 Trustees of Indiana University

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// A simple example of using read_graphviz to load a GraphViz Dot textual
// graph into a BGL adjacency_list graph

// Author: Ronald Garcia


#include <boost/graph/graphviz.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <string>
#include <sstream>

using namespace boost;
using namespace std;

int main() {
  // Vertex properties
  typedef property < vertex_name_t, std::string,
            property < vertex_color_t, float > > vertex_p;  
  // Edge properties
  typedef property < edge_weight_t, double > edge_p;
  // Graph properties
  typedef property < graph_name_t, std::string > graph_p;
  // adjacency_list-based type
  typedef adjacency_list < vecS, vecS, directedS,
    vertex_p, edge_p, graph_p > graph_t;

  // Construct an empty graph and prepare the dynamic_property_maps.
  graph_t graph(0);
  dynamic_properties dp;

  property_map<graph_t, vertex_name_t>::type name =
    get(vertex_name, graph);
  dp.property("node_id",name);

  property_map<graph_t, vertex_color_t>::type mass =
    get(vertex_color, graph);
  dp.property("mass",mass);

  property_map<graph_t, edge_weight_t>::type weight =
    get(edge_weight, graph);
  dp.property("weight",weight);

  // Use ref_property_map to turn a graph property into a property map
  boost::ref_property_map<graph_t*,std::string> 
    gname(get_property(graph,graph_name));
  dp.property("name",gname);

  // Sample graph as an std::istream;
  std::istringstream
    gvgraph("digraph { graph [name=\"graphname\"]  a  c e [mass = 6.66] }");

  bool status = read_graphviz(gvgraph,graph,dp,"node_id");

  return 0;
}
