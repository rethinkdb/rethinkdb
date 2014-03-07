//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Copyright 2009 Trustees of Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek, Michael Hansen
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <iostream>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/incremental_components.hpp>
#include <boost/pending/disjoint_sets.hpp>

/*

  This example shows how to use the disjoint set data structure
  to compute the connected components of an undirected, changing
  graph.

  Sample output:

  An undirected graph:
  0 <--> 1 4 
  1 <--> 0 4 
  2 <--> 5 
  3 <--> 
  4 <--> 1 0 
  5 <--> 2 

  representative[0] = 1
  representative[1] = 1
  representative[2] = 5
  representative[3] = 3
  representative[4] = 1
  representative[5] = 5

  component 0 contains: 4 1 0 
  component 1 contains: 3 
  component 2 contains: 5 2 

 */

using namespace boost;

int main(int argc, char* argv[]) 
{
  typedef adjacency_list <vecS, vecS, undirectedS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor Vertex;
  typedef graph_traits<Graph>::vertices_size_type VertexIndex;

  const int VERTEX_COUNT = 6;
  Graph graph(VERTEX_COUNT);

  std::vector<VertexIndex> rank(num_vertices(graph));
  std::vector<Vertex> parent(num_vertices(graph));

  typedef VertexIndex* Rank;
  typedef Vertex* Parent;

  disjoint_sets<Rank, Parent> ds(&rank[0], &parent[0]);

  initialize_incremental_components(graph, ds);
  incremental_components(graph, ds);

  graph_traits<Graph>::edge_descriptor edge;
  bool flag;

  boost::tie(edge, flag) = add_edge(0, 1, graph);
  ds.union_set(0,1);

  boost::tie(edge, flag) = add_edge(1, 4, graph);
  ds.union_set(1,4);

  boost::tie(edge, flag) = add_edge(4, 0, graph);
  ds.union_set(4,0);

  boost::tie(edge, flag) = add_edge(2, 5, graph);
  ds.union_set(2,5);
    
  std::cout << "An undirected graph:" << std::endl;
  print_graph(graph, get(boost::vertex_index, graph));
  std::cout << std::endl;
    
  BOOST_FOREACH(Vertex current_vertex, vertices(graph)) {
    std::cout << "representative[" << current_vertex << "] = " <<
      ds.find_set(current_vertex) << std::endl;
  }

  std::cout << std::endl;

  typedef component_index<VertexIndex> Components;

  // NOTE: Because we're using vecS for the graph type, we're
  // effectively using identity_property_map for a vertex index map.
  // If we were to use listS instead, the index map would need to be
  // explicitly passed to the component_index constructor.
  Components components(parent.begin(), parent.end());

  // Iterate through the component indices
  BOOST_FOREACH(VertexIndex current_index, components) {
    std::cout << "component " << current_index << " contains: ";

    // Iterate through the child vertex indices for [current_index]
    BOOST_FOREACH(VertexIndex child_index,
                  components[current_index]) {
      std::cout << child_index << " ";
    }

    std::cout << std::endl;
  }

  return (0);
}

