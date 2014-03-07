//=======================================================================
// Copyright 2009 Trustees of Indiana University.
// Authors: Michael Hansen, Andrew Lumsdaine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>
#include <map>
#include <set>

#include <boost/foreach.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/incremental_components.hpp>
#include <boost/graph/random.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/random.hpp>
#include <boost/test/minimal.hpp>

using namespace boost;

typedef adjacency_list<vecS, vecS, undirectedS> VectorGraph;

typedef adjacency_list<listS, listS, undirectedS,
                       property<vertex_index_t, unsigned int> > ListGraph;

template <typename Graph>
void test_graph(const Graph& graph) {
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<Graph>::vertices_size_type vertices_size_type;

  typedef typename property_map<Graph, vertex_index_t>::type IndexPropertyMap;

  typedef std::map<vertex_descriptor, vertices_size_type> RankMap;
  typedef associative_property_map<RankMap> RankPropertyMap;

  typedef std::vector<vertex_descriptor> ParentMap;
  typedef iterator_property_map<typename ParentMap::iterator,
    IndexPropertyMap, vertex_descriptor, vertex_descriptor&> IndexParentMap;

  RankMap rank_map;
  RankPropertyMap rank_property_map(rank_map);

  ParentMap parent_map(num_vertices(graph));  
  IndexParentMap index_parent_map(parent_map.begin());

  // Create disjoint sets of vertices from the graph
  disjoint_sets<RankPropertyMap, IndexParentMap>
    vertex_sets(rank_property_map, index_parent_map);

  initialize_incremental_components(graph, vertex_sets);
  incremental_components(graph, vertex_sets);

  // Build component index from the graph's vertices, its index map,
  // and the disjoint sets.
  typedef component_index<vertices_size_type> Components;
  Components vertex_components(parent_map.begin(),
                               parent_map.end(),
                               get(boost::vertex_index, graph));

  // Create a reverse-lookup map for vertex indices
  std::vector<vertex_descriptor> reverse_index_map(num_vertices(graph));

  BOOST_FOREACH(vertex_descriptor vertex, vertices(graph)) {
    reverse_index_map[get(get(boost::vertex_index, graph), vertex)] = vertex;
  }

  // Verify that components are really connected
  BOOST_FOREACH(vertices_size_type component_index,
                vertex_components) {

    std::set<vertex_descriptor> component_vertices;

    BOOST_FOREACH(vertices_size_type child_index,
                  vertex_components[component_index]) {

      vertex_descriptor child_vertex = reverse_index_map[child_index];
      component_vertices.insert(child_vertex);
      
    } // foreach child_index

    // Verify that children are connected to each other in some
    // manner, but not to vertices outside their component.
    BOOST_FOREACH(vertex_descriptor child_vertex,
                  component_vertices) {

      // Skip orphan vertices
      if (out_degree(child_vertex, graph) == 0) {
        continue;
      }

      // Make sure at least one edge exists between [child_vertex] and
      // another vertex in the component.
      bool edge_exists = false;

      BOOST_FOREACH(edge_descriptor child_edge,
                    out_edges(child_vertex, graph)) {

        if (component_vertices.count(target(child_edge, graph)) > 0) {
          edge_exists = true;
          break;
        }

      } // foreach child_edge

      BOOST_REQUIRE(edge_exists);

    } // foreach child_vertex

  } // foreach component_index

} // test_graph

int test_main(int argc, char* argv[])
{
  std::size_t vertices_to_generate = 100,
    edges_to_generate = 50,
    random_seed = time(0);

  // Parse command-line arguments

  if (argc > 1) {
    vertices_to_generate = lexical_cast<std::size_t>(argv[1]);
  }

  if (argc > 2) {
    edges_to_generate = lexical_cast<std::size_t>(argv[2]);
  }

  if (argc > 3) {
    random_seed = lexical_cast<std::size_t>(argv[3]);
  }

  minstd_rand generator(random_seed);

  // Generate random vector and list graphs
  VectorGraph vector_graph;
  generate_random_graph(vector_graph, vertices_to_generate,
                        edges_to_generate, generator, false);

  test_graph(vector_graph);

  ListGraph list_graph;
  generate_random_graph(list_graph, vertices_to_generate,
                        edges_to_generate, generator, false);

  // Assign indices to list_graph's vertices
  graph_traits<ListGraph>::vertices_size_type index = 0;
  BOOST_FOREACH(graph_traits<ListGraph>::vertex_descriptor vertex,
                vertices(list_graph)) {
    put(get(boost::vertex_index, list_graph), vertex, index++);
  }

  test_graph(list_graph); 

  return 0;

}
