//  (C) Copyright Jeremy Siek 2004 
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <iostream>
#include <boost/cstdlib.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/subgraph.hpp>

int
main()
{
  using namespace boost;
  using std::string;

  typedef adjacency_list<vecS, vecS, directedS,no_property, 
    property<edge_index_t, int>,
    property<graph_name_t, string> > graph_t;

  graph_t g;
  get_property(g, graph_name) = "graph";

  std::cout << "name: " << get_property(g, graph_name) << std::endl;

  typedef subgraph<graph_t> subgraph_t;

  subgraph_t sg;
  get_property(sg, graph_name) = "subgraph";

  std::cout << "name: " << get_property(sg, graph_name) << std::endl;
  
  return exit_success;
}
