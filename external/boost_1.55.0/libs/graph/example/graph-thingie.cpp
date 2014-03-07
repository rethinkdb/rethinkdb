// Copyright 2008 Trustees of Indiana University

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// An example of using read_graphviz to load a GraphViz Dot textual
// graph into a BGL adjacency_list graph that has custom properties.

// Author: Ronald Garcia



#include <boost/graph/graphviz.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <sstream>

using namespace boost;
using namespace std;


//
// Create a custom graph properties
//  (see the documentation for adjacency list)
struct graph_identifier_t { typedef graph_property_tag kind; };
struct vertex_label_t { typedef vertex_property_tag kind; };

int main() {

  // Vertex properties
  typedef property < vertex_name_t, string,
            property < vertex_label_t, string,
              property < vertex_root_t, int > > > vertex_p;  
  // Edge properties
  typedef property < edge_name_t, string > edge_p;
  // Graph properties
  typedef property < graph_name_t, string,
            property < graph_identifier_t, string > > graph_p;
  // adjacency_list-based type
  typedef adjacency_list < vecS, vecS, directedS,
    vertex_p, edge_p, graph_p > graph_t;

  // Construct an empty graph and prepare the dynamic_property_maps.
  graph_t graph(0);
  dynamic_properties dp;

  property_map<graph_t, vertex_name_t>::type vname =
    get(vertex_name, graph);
  dp.property("node_id",vname);

  property_map<graph_t, vertex_label_t>::type vlabel =
    get(vertex_label_t(), graph);
  dp.property("label",vlabel);

  property_map<graph_t, vertex_root_t>::type root =
    get(vertex_root, graph);
  dp.property("root",root);

  property_map<graph_t, edge_name_t>::type elabel =
    get(edge_name, graph);
  dp.property("label",elabel);

  // Use ref_property_map to turn a graph property into a property map
  ref_property_map<graph_t*,string> 
    gname(get_property(graph,graph_name));
  dp.property("name",gname);

  // Use ref_property_map to turn a graph property into a property map
  ref_property_map<graph_t*,string> 
    gid(get_property(graph,graph_identifier_t()));
  dp.property("identifier",gid);
  // Sample graph as an istream;

const char* dot = 
"digraph \
{ \
  graph [name=\"GRAPH\", identifier=\"CX2A1Z\"] \
    \
    a [label=\"NODE_A\", root=\"1\"] \
    b [label=\"NODE_B\", root=\"0\"] \
 \
 a -> b [label=\"EDGE_1\"] \
 b -> c [label=\"EDGE_2\"] \
}";


  istringstream gvgraph(dot);

  bool status = read_graphviz(gvgraph,graph,dp,"node_id");

  cout << "graph " << get("name",dp,&graph) <<
      " (" << get("identifier",dp,&graph) << ")\n\n";

  BOOST_FOREACH( graph_t::vertex_descriptor v, vertices(graph) ) {
    cout << "vertex " << get("node_id",dp,v) <<
      " (" << get("label",dp,v) << ")\n";
  }

  return 0;
}
