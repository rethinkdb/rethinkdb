//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
//  Sample output
//
// DFS categorized directed graph
// tree: 0 --> 2
// tree: 2 --> 1
// back: 1 --> 1
// tree: 1 --> 3
// back: 3 --> 1
// tree: 3 --> 4
// back: 4 --> 0
// back: 4 --> 1
// forward or cross: 2 --> 3

// BFS categorized directed graph
// tree: 0 --> 2
// tree: 2 --> 1
// tree: 2 --> 3
// cycle: 1 --> 1
// cycle: 1 --> 3
// cycle: 3 --> 1
// tree: 3 --> 4
// cycle: 4 --> 0
// cycle: 4 --> 1

#include <boost/config.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <string>

#include <boost/graph/visitors.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/depth_first_search.hpp>

using namespace boost;
using namespace std;

template <class Tag>
struct edge_printer : public base_visitor<edge_printer<Tag> > {
  typedef Tag event_filter;
  edge_printer(std::string edge_t) : m_edge_type(edge_t) { }
  template <class Edge, class Graph>
  void operator()(Edge e, Graph& G) {
    std::cout << m_edge_type << ": " << source(e, G) 
              << " --> " <<  target(e, G) << std::endl;
  }
  std::string m_edge_type;
};
template <class Tag>
edge_printer<Tag> print_edge(std::string type, Tag) { 
  return edge_printer<Tag>(type);
}

int 
main(int, char*[])
{

  using namespace boost;
  
  typedef adjacency_list<> Graph;
  typedef std::pair<int,int> E;
  E edges[] = { E(0, 2),
                E(1, 1), E(1, 3),
                E(2, 1), E(2, 3),
                E(3, 1), E(3, 4),
                E(4, 0), E(4, 1) };  
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  Graph G(5);
  for (std::size_t j = 0; j < sizeof(edges)/sizeof(E); ++j)
    add_edge(edges[j].first, edges[j].second, G);
#else
  Graph G(edges, edges + sizeof(edges)/sizeof(E), 5);
#endif

  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::vertices_size_type size_type;
  
  std::vector<size_type> d(num_vertices(G));  
  std::vector<size_type> f(num_vertices(G));

  cout << "DFS categorized directed graph" << endl;
  depth_first_search(G, visitor(make_dfs_visitor(
      make_list(print_edge("tree", on_tree_edge()),
                print_edge("back", on_back_edge()),
                print_edge("forward or cross", on_forward_or_cross_edge())
                ))));

  cout << endl << "BFS categorized directed graph" << endl;
  boost::breadth_first_search
    (G, vertex(0, G), visitor(make_bfs_visitor(
     std::make_pair(print_edge("tree", on_tree_edge()),
                    print_edge("cycle", on_non_tree_edge())))));

  return 0;
}

