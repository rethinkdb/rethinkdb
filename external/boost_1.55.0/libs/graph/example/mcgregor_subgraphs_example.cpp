//=======================================================================
// Copyright 2009 Trustees of Indiana University.
// Authors: Michael Hansen
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <fstream>
#include <iostream>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/mcgregor_common_subgraphs.hpp>
#include <boost/property_map/shared_array_property_map.hpp>

using namespace boost;

// Callback that looks for the first common subgraph whose size
// matches the user's preference.
template <typename Graph>
struct example_callback {

  typedef typename graph_traits<Graph>::vertices_size_type VertexSizeFirst;

  example_callback(const Graph& graph1) :
    m_graph1(graph1) { }

  template <typename CorrespondenceMapFirstToSecond,
            typename CorrespondenceMapSecondToFirst>
  bool operator()(CorrespondenceMapFirstToSecond correspondence_map_1_to_2,
                  CorrespondenceMapSecondToFirst correspondence_map_2_to_1,
                  VertexSizeFirst subgraph_size) {

    // Fill membership map for first graph
    typedef typename property_map<Graph, vertex_index_t>::type VertexIndexMap;
    typedef shared_array_property_map<bool, VertexIndexMap> MembershipMap;
      
    MembershipMap membership_map1(num_vertices(m_graph1),
                                  get(vertex_index, m_graph1));

    fill_membership_map<Graph>(m_graph1, correspondence_map_1_to_2, membership_map1);

    // Generate filtered graphs using membership map
    typedef typename membership_filtered_graph_traits<Graph, MembershipMap>::graph_type
      MembershipFilteredGraph;

    MembershipFilteredGraph subgraph1 =
      make_membership_filtered_graph(m_graph1, membership_map1);

    // Print the graph out to the console
    std::cout << "Found common subgraph (size " << subgraph_size << ")" << std::endl;
    print_graph(subgraph1);
    std::cout << std::endl;

    // Explore the entire space
    return (true);
  }

private:
  const Graph& m_graph1;
  VertexSizeFirst m_max_subgraph_size;
};

int main (int argc, char *argv[]) {

  // Using a vecS graph here so that we don't have to mess around with
  // a vertex index map; it will be implicit.
  typedef adjacency_list<listS, vecS, directedS,
    property<vertex_name_t, unsigned int,
    property<vertex_index_t, unsigned int> >,
    property<edge_name_t, unsigned int> > Graph;

  typedef property_map<Graph, vertex_name_t>::type VertexNameMap;

  // Test maximum and unique variants on known graphs
  Graph graph_simple1, graph_simple2;
  example_callback<Graph> user_callback(graph_simple1);

  VertexNameMap vname_map_simple1 = get(vertex_name, graph_simple1);
  VertexNameMap vname_map_simple2 = get(vertex_name, graph_simple2);

  // Graph that looks like a triangle
  put(vname_map_simple1, add_vertex(graph_simple1), 1);
  put(vname_map_simple1, add_vertex(graph_simple1), 2);
  put(vname_map_simple1, add_vertex(graph_simple1), 3);

  add_edge(0, 1, graph_simple1);
  add_edge(0, 2, graph_simple1);
  add_edge(1, 2, graph_simple1);

  std::cout << "First graph:" << std::endl;
  print_graph(graph_simple1);
  std::cout << std::endl;

  // Triangle with an extra vertex
  put(vname_map_simple2, add_vertex(graph_simple2), 1);
  put(vname_map_simple2, add_vertex(graph_simple2), 2);
  put(vname_map_simple2, add_vertex(graph_simple2), 3);
  put(vname_map_simple2, add_vertex(graph_simple2), 4);

  add_edge(0, 1, graph_simple2);
  add_edge(0, 2, graph_simple2);
  add_edge(1, 2, graph_simple2);
  add_edge(1, 3, graph_simple2);

  std::cout << "Second graph:" << std::endl;
  print_graph(graph_simple2);
  std::cout << std::endl;

  // All subgraphs
  std::cout << "mcgregor_common_subgraphs:" << std::endl;
  mcgregor_common_subgraphs
    (graph_simple1, graph_simple2, true, user_callback,
     vertices_equivalent(make_property_map_equivalent(vname_map_simple1, vname_map_simple2))); 
  std::cout << std::endl;

  // Unique subgraphs
  std::cout << "mcgregor_common_subgraphs_unique:" << std::endl;
  mcgregor_common_subgraphs_unique
    (graph_simple1, graph_simple2, true, user_callback,
     vertices_equivalent(make_property_map_equivalent(vname_map_simple1, vname_map_simple2))); 
  std::cout << std::endl;

  // Maximum subgraphs
  std::cout << "mcgregor_common_subgraphs_maximum:" << std::endl;
  mcgregor_common_subgraphs_maximum
    (graph_simple1, graph_simple2, true, user_callback,
     vertices_equivalent(make_property_map_equivalent(vname_map_simple1, vname_map_simple2))); 
  std::cout << std::endl;

  // Maximum, unique subgraphs
  std::cout << "mcgregor_common_subgraphs_maximum_unique:" << std::endl;
  mcgregor_common_subgraphs_maximum_unique
    (graph_simple1, graph_simple2, true, user_callback,
     vertices_equivalent(make_property_map_equivalent(vname_map_simple1, vname_map_simple2))); 

  return 0;
}
