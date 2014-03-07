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

#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/graph/is_kuratowski_subgraph.hpp>

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

  // Create a K_6 (complete graph on 6 vertices), which
  // contains both Kuratowski subgraphs as minors.
  graph g(6);
  add_edge(0,1,g);
  add_edge(0,2,g);
  add_edge(0,3,g);
  add_edge(0,4,g);
  add_edge(0,5,g);
  add_edge(1,2,g);
  add_edge(1,3,g);
  add_edge(1,4,g);
  add_edge(1,5,g);
  add_edge(2,3,g);
  add_edge(2,4,g);
  add_edge(2,5,g);
  add_edge(3,4,g);
  add_edge(3,5,g);
  add_edge(4,5,g);


  // Initialize the interior edge index
  property_map<graph, edge_index_t>::type e_index = get(edge_index, g);
  graph_traits<graph>::edges_size_type edge_count = 0;
  graph_traits<graph>::edge_iterator ei, ei_end;
  for(boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    put(e_index, *ei, edge_count++);
  

  // Test for planarity - we know it is not planar, we just want to 
  // compute the kuratowski subgraph as a side-effect
  typedef std::vector< graph_traits<graph>::edge_descriptor > 
    kuratowski_edges_t;
  kuratowski_edges_t kuratowski_edges;
  if (boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
                                   boyer_myrvold_params::kuratowski_subgraph = 
                                       std::back_inserter(kuratowski_edges)
                                   )
      )
    std::cout << "Input graph is planar" << std::endl;
  else
    {
      std::cout << "Input graph is not planar" << std::endl;

      std::cout << "Edges in the Kuratowski subgraph: ";
      kuratowski_edges_t::iterator ki, ki_end;
      ki_end = kuratowski_edges.end();
      for(ki = kuratowski_edges.begin(); ki != ki_end; ++ki)
        {
          std::cout << *ki << " ";
        }
      std::cout << std::endl;

      std::cout << "Is a kuratowski subgraph? ";
      if (is_kuratowski_subgraph
          (g, kuratowski_edges.begin(), kuratowski_edges.end())
          )
        std::cout << "Yes." << std::endl;
      else
        std::cout << "No." << std::endl;
    }

  return 0;
}
