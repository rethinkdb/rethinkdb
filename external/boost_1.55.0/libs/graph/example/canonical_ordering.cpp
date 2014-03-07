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

#include <boost/graph/planar_canonical_ordering.hpp>
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

  // Create a maximal planar graph on 6 vertices
  graph g(6);

  add_edge(0,1,g);
  add_edge(1,2,g);
  add_edge(2,3,g);
  add_edge(3,4,g);
  add_edge(4,5,g);
  add_edge(5,0,g);

  add_edge(0,2,g);
  add_edge(0,3,g);
  add_edge(0,4,g);

  add_edge(1,3,g);
  add_edge(1,4,g);
  add_edge(1,5,g);

  // Initialize the interior edge index
  property_map<graph, edge_index_t>::type e_index = get(edge_index, g);
  graph_traits<graph>::edges_size_type edge_count = 0;
  graph_traits<graph>::edge_iterator ei, ei_end;
  for(boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    put(e_index, *ei, edge_count++);
  

  // Test for planarity - we know it is planar, we just want to 
  // compute the planar embedding as a side-effect
  typedef std::vector< graph_traits<graph>::edge_descriptor > vec_t;
  std::vector<vec_t> embedding(num_vertices(g));
  if (boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
                                   boyer_myrvold_params::embedding = 
                                       make_iterator_property_map(
                                         embedding.begin(), get(vertex_index, g))
                                   )
      )
    std::cout << "Input graph is planar" << std::endl;
  else
    std::cout << "Input graph is not planar" << std::endl;

  typedef std::vector<graph_traits<graph>::vertex_descriptor> 
    ordering_storage_t;
  
  ordering_storage_t ordering;
  planar_canonical_ordering(g,
                            make_iterator_property_map(
                              embedding.begin(), get(vertex_index, g)),
                            std::back_inserter(ordering));

  ordering_storage_t::iterator oi, oi_end;
  oi_end = ordering.end();
  std::cout << "The planar canonical ordering is: ";
  for(oi = ordering.begin(); oi != oi_end; ++oi)
    std::cout << *oi << " ";
  std::cout << std::endl;

  return 0;  
}
