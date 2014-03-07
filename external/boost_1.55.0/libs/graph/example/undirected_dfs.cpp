//=======================================================================
// Copyright 2002 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <string>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/undirected_dfs.hpp>
#include <boost/cstdlib.hpp>
#include <iostream>

/*
  Example graph from Tarjei Knapstad.

                   H15
                   |
          H8       C2
            \     /  \
          H9-C0-C1    C3-O7-H14
            /   |     |
          H10   C6    C4
               /  \  /  \
              H11  C5    H13
                   |
                   H12
*/

std::string name[] = { "C0", "C1", "C2", "C3", "C4", "C5", "C6", "O7",
                       "H8", "H9", "H10", "H11", "H12", "H13", "H14", "H15"};


struct detect_loops : public boost::dfs_visitor<>
{
  template <class Edge, class Graph>
  void back_edge(Edge e, const Graph& g) {
    std::cout << name[source(e, g)]
              << " -- "
              << name[target(e, g)] << "\n";
  }
};

int main(int, char*[])
{
  using namespace boost;
  typedef adjacency_list< vecS, vecS, undirectedS,
    no_property,
    property<edge_color_t, default_color_type> > graph_t;
  typedef graph_traits<graph_t>::vertex_descriptor vertex_t;
  
  const std::size_t N = sizeof(name)/sizeof(std::string);
  graph_t g(N);
  
  add_edge(0, 1, g);
  add_edge(0, 8, g);
  add_edge(0, 9, g);
  add_edge(0, 10, g);
  add_edge(1, 2, g);
  add_edge(1, 6, g);
  add_edge(2, 15, g);
  add_edge(2, 3, g);
  add_edge(3, 7, g);
  add_edge(3, 4, g);
  add_edge(4, 13, g);
  add_edge(4, 5, g);
  add_edge(5, 12, g);
  add_edge(5, 6, g);
  add_edge(6, 11, g);
  add_edge(7, 14, g);
  
  std::cout << "back edges:\n";
  detect_loops vis;
  undirected_dfs(g, root_vertex(vertex_t(0)).visitor(vis)
                 .edge_color_map(get(edge_color, g)));
  std::cout << std::endl;
  
  return boost::exit_success;
}
