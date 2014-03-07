//=======================================================================
// Copyright (C) 2012 Flavio De Lorenzi (fdlorenzi@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/vf2_sub_graph_iso.hpp>
using namespace boost;

int main() {
  typedef property<edge_name_t, char> edge_property;
  typedef property<vertex_name_t, char, property<vertex_index_t, int> > vertex_property;

  // Using a vecS graphs => the index maps are implicit.
  typedef adjacency_list<vecS, vecS, bidirectionalS, vertex_property, edge_property> graph_type;
  
  // Build graph1
  graph_type graph1;
  
  add_vertex(vertex_property('a'), graph1);
  add_vertex(vertex_property('a'), graph1);
  add_vertex(vertex_property('a'), graph1);
  
  add_edge(0, 1, edge_property('b'), graph1); 
  add_edge(0, 1, edge_property('b'), graph1); 
  add_edge(0, 1, edge_property('d'), graph1); 
  
  add_edge(1, 2, edge_property('s'), graph1); 
  
  add_edge(2, 2, edge_property('l'), graph1); 
  add_edge(2, 2, edge_property('l'), graph1); 
  
  // Build graph2
  graph_type graph2;
  
  add_vertex(vertex_property('a'), graph2);
  add_vertex(vertex_property('a'), graph2);
  add_vertex(vertex_property('a'), graph2);
  add_vertex(vertex_property('a'), graph2);
  add_vertex(vertex_property('a'), graph2);
  add_vertex(vertex_property('a'), graph2);
  
  add_edge(0, 1, edge_property('a'), graph2); 
  add_edge(0, 1, edge_property('a'), graph2); 
  add_edge(0, 1, edge_property('b'), graph2); 

  add_edge(1, 2, edge_property('s'), graph2); 
  
  add_edge(2, 3, edge_property('b'), graph2); 
  add_edge(2, 3, edge_property('d'), graph2); 
  add_edge(2, 3, edge_property('b'), graph2); 
  
  add_edge(3, 4, edge_property('s'), graph2); 
  
  add_edge(4, 4, edge_property('l'), graph2); 
  add_edge(4, 4, edge_property('l'), graph2); 

  add_edge(4, 5, edge_property('c'), graph2); 
  add_edge(4, 5, edge_property('c'), graph2); 
  add_edge(4, 5, edge_property('c'), graph2); 
  
  add_edge(5, 0, edge_property('s'), graph2); 
  
  // create predicates
  typedef property_map<graph_type, vertex_name_t>::type vertex_name_map_t;
  typedef property_map_equivalent<vertex_name_map_t, vertex_name_map_t> vertex_comp_t;
  vertex_comp_t vertex_comp =
    make_property_map_equivalent(get(vertex_name, graph1), get(vertex_name, graph2));
  
  typedef property_map<graph_type, edge_name_t>::type edge_name_map_t;
  typedef property_map_equivalent<edge_name_map_t, edge_name_map_t> edge_comp_t;
  edge_comp_t edge_comp =
    make_property_map_equivalent(get(edge_name, graph1), get(edge_name, graph2));
 
  // Create callback
  vf2_print_callback<graph_type, graph_type> callback(graph1, graph2);

  // Print out all subgraph isomorphism mappings between graph1 and graph2.
  // Function vertex_order_by_mult is used to compute the order of 
  // vertices of graph1. This is the order in which the vertices are examined
  // during the matching process.
  vf2_subgraph_iso(graph1, graph2, callback, vertex_order_by_mult(graph1),
                   edges_equivalent(edge_comp).vertices_equivalent(vertex_comp));
  
  return 0;
}
