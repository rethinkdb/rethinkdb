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

#include <boost/graph/make_biconnected_planar.hpp>
#include <boost/graph/make_maximal_planar.hpp>
#include <boost/graph/planar_face_traversal.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>



// This example shows how to start with a connected planar graph 
// and add edges to make the graph maximal planar (triangulated.)
// Any maximal planar simple graph on n vertices has 3n - 6 edges and 
// 2n - 4 faces, a consequence of Euler's formula.



using namespace boost;


// This visitor is passed to planar_face_traversal to count the 
// number of faces.
struct face_counter : public planar_face_traversal_visitor
{
  face_counter() : count(0) {}
  void begin_face() { ++count; }
  int count;
};


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

  // Create the graph - a straight line
  graph g(10);
  add_edge(0,1,g);
  add_edge(1,2,g);
  add_edge(2,3,g);
  add_edge(3,4,g);
  add_edge(4,5,g);
  add_edge(5,6,g);
  add_edge(6,7,g);
  add_edge(7,8,g);
  add_edge(8,9,g);

  std::cout << "Since the input graph is planar with " << num_vertices(g) 
            << " vertices," << std::endl
            << "The output graph should be planar with " 
            << 3*num_vertices(g) - 6 << " edges and "
            << 2*num_vertices(g) - 4 << " faces." << std::endl;

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
  
  make_biconnected_planar(g, &embedding[0]);

  // Re-initialize the edge index, since we just added a few edges
  edge_count = 0;
  for(boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    put(e_index, *ei, edge_count++);


  //Test for planarity again; compute the planar embedding as a side-effect
  if (boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
                                   boyer_myrvold_params::embedding = 
                                       &embedding[0]
                                   )
      )
    std::cout << "After calling make_biconnected, the graph is still planar" 
              << std::endl;
  else
    std::cout << "After calling make_biconnected, the graph is not planar" 
              << std::endl;

  make_maximal_planar(g, &embedding[0]);



  // Re-initialize the edge index, since we just added a few edges
  edge_count = 0;
  for(boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    put(e_index, *ei, edge_count++);

  // Test for planarity one final time; compute the planar embedding as a 
  // side-effect
  std::cout << "After calling make_maximal_planar, the final graph ";
  if (boyer_myrvold_planarity_test(boyer_myrvold_params::graph = g,
                                   boyer_myrvold_params::embedding = 
                                       &embedding[0]
                                   )
      )
    std::cout << "is planar." << std::endl;
  else
    std::cout << "is not planar." << std::endl;
  
  std::cout << "The final graph has " << num_edges(g) 
            << " edges." << std::endl;

  face_counter count_visitor;
  planar_face_traversal(g, &embedding[0], count_visitor);
  std::cout << "The final graph has " << count_visitor.count << " faces." 
            << std::endl;

  return 0;
}
