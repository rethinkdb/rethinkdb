//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/ref.hpp>
#include <vector>

#include <boost/graph/biconnected_components.hpp>
#include <boost/graph/make_biconnected_planar.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>


using namespace boost;

int main(int argc, char** argv)
{
  
  typedef adjacency_list
    < vecS,
      vecS,
      undirectedS,
      property<vertex_index_t, int>,
      property<edge_index_t, int>
    > 
    graph;

  graph g(11);
  add_edge(0,1,g);
  add_edge(2,3,g);
  add_edge(3,0,g);
  add_edge(3,4,g);
  add_edge(4,5,g);
  add_edge(5,3,g);
  add_edge(5,6,g);
  add_edge(6,7,g);
  add_edge(7,8,g);
  add_edge(8,5,g);
  add_edge(8,9,g);
  add_edge(0,10,g);


  //Initialize the interior edge index
  property_map<graph, edge_index_t>::type e_index = get(edge_index, g);
  graph_traits<graph>::edges_size_type edge_count = 0;
  graph_traits<graph>::edge_iterator ei, ei_end;
  for(boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    put(e_index, *ei, edge_count++);
  
  
  //Test for planarity; compute the planar embedding as a side-effect
  typedef std::vector< graph_traits<graph>::edge_descriptor > vec_t;
  std::vector<vec_t> embedding(num_vertices(g));
  if (boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
                                   boyer_myrvold_params::embedding = 
                                     &embedding[0]
                                   )
      )
    std::cout << "Input graph is planar" << std::endl;
  else
    std::cout << "Input graph is not planar" << std::endl;
  
  typedef std::vector< graph_traits<graph>::edges_size_type > 
    component_storage_t;
  typedef iterator_property_map
    < component_storage_t::iterator, 
      property_map<graph, edge_index_t>::type
    >
    component_map_t;
  
  component_storage_t component_storage(num_edges(g));
  component_map_t component(component_storage.begin(), get(edge_index, g));
  
  std::cout << "Before calling make_biconnected_planar, the graph has "
            << biconnected_components(g, component)
            << " biconnected components" << std::endl;
  
  make_biconnected_planar(g, &embedding[0]);

  // Re-initialize the edge index, since we just added a few edges
  edge_count = 0;
  for(boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    put(e_index, *ei, edge_count++);

  // Re-size the storage for the biconnected components, since we
  // just added a few edges
  
  component_storage.resize(num_edges(g));
  component = component_map_t(component_storage.begin(), get(edge_index,g));

  std::cout << "After calling make_biconnected_planar, the graph has "
            << biconnected_components(g, component)
            << " biconnected components" << std::endl;

  if (boyer_myrvold_planarity_test(g))
    std::cout << "Also, the graph is still planar." << std::endl;
  else
    std::cout << "But the graph is not still planar." << std::endl;

  return 0;  
}
