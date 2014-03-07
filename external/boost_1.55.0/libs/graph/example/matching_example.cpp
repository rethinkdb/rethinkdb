//=======================================================================
// Copyright (c) 2005 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//=======================================================================
#include <string>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <cassert>

#include <boost/graph/max_cardinality_matching.hpp>


using namespace boost;

typedef adjacency_list<vecS, vecS, undirectedS> my_graph; 

int main()
{

  // Create the following graph: (it'll look better when output
  // to the terminal in a fixed width font...)

  const int n_vertices = 18;

  std::vector<std::string> ascii_graph;

  ascii_graph.push_back("           0       1---2       3       ");
  ascii_graph.push_back("            \\     /     \\     /        ");
  ascii_graph.push_back("             4---5       6---7         ");
  ascii_graph.push_back("             |   |       |   |         ");
  ascii_graph.push_back("             8---9      10---11        ");
  ascii_graph.push_back("            /     \\     /     \\        ");
  ascii_graph.push_back("     12   13      14---15      16   17 ");

  // It has a perfect matching of size 8. There are two isolated
  // vertices that we'll use later...

  my_graph g(n_vertices);
  
  // our vertices are stored in a vector, so we can refer to vertices
  // by integers in the range 0..15

  add_edge(1,2,g);
  add_edge(0,4,g);
  add_edge(1,5,g);
  add_edge(2,6,g);
  add_edge(3,7,g);
  add_edge(4,5,g);
  add_edge(6,7,g);
  add_edge(4,8,g);
  add_edge(5,9,g);
  add_edge(6,10,g);
  add_edge(7,11,g);
  add_edge(8,9,g);
  add_edge(10,11,g);
  add_edge(8,13,g);
  add_edge(9,14,g);
  add_edge(10,15,g);
  add_edge(11,16,g);
  add_edge(14,15,g);

  std::vector<graph_traits<my_graph>::vertex_descriptor> mate(n_vertices);

  // find the maximum cardinality matching. we'll use a checked version
  // of the algorithm, which takes a little longer than the unchecked
  // version, but has the advantage that it will return "false" if the
  // matching returned is not actually a maximum cardinality matching
  // in the graph.

  bool success = checked_edmonds_maximum_cardinality_matching(g, &mate[0]);
  assert(success);

  std::cout << "In the following graph:" << std::endl << std::endl;

  for(std::vector<std::string>::iterator itr = ascii_graph.begin(); itr != ascii_graph.end(); ++itr)
    std::cout << *itr << std::endl;

  std::cout << std::endl << "Found a matching of size " << matching_size(g, &mate[0]) << std::endl;

  std::cout << "The matching is:" << std::endl;
  
  graph_traits<my_graph>::vertex_iterator vi, vi_end;
  for(boost::tie(vi,vi_end) = vertices(g); vi != vi_end; ++vi)
    if (mate[*vi] != graph_traits<my_graph>::null_vertex() && *vi < mate[*vi])
      std::cout << "{" << *vi << ", " << mate[*vi] << "}" << std::endl;

  std::cout << std::endl;

  //now we'll add two edges, and the perfect matching has size 9

  ascii_graph.pop_back();
  ascii_graph.push_back("     12---13      14---15      16---17 ");

  add_edge(12,13,g);
  add_edge(16,17,g);

  success = checked_edmonds_maximum_cardinality_matching(g, &mate[0]);
  assert(success);

  std::cout << "In the following graph:" << std::endl << std::endl;

  for(std::vector<std::string>::iterator itr = ascii_graph.begin(); itr != ascii_graph.end(); ++itr)
    std::cout << *itr << std::endl;

  std::cout << std::endl << "Found a matching of size " << matching_size(g, &mate[0]) << std::endl;

  std::cout << "The matching is:" << std::endl;
  
  for(boost::tie(vi,vi_end) = vertices(g); vi != vi_end; ++vi)
    if (mate[*vi] != graph_traits<my_graph>::null_vertex() && *vi < mate[*vi])
      std::cout << "{" << *vi << ", " << mate[*vi] << "}" << std::endl;

  return 0;
}
