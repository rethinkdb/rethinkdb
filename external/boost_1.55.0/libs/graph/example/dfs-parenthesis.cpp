//=======================================================================
// Copyright 2001 Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee, 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/graph/graphviz.hpp>
#include <boost/graph/depth_first_search.hpp>

char name[] = "abcdefghij";

struct parenthesis_visitor : public boost::default_dfs_visitor
{
  template <class Vertex, class Graph> void
  start_vertex(Vertex v, const Graph &)
  {
    std::cout << ' ';
  }
  template <class Vertex, class Graph> void
  discover_vertex(Vertex v, const Graph &)
  {
    std::cout << "(" << name[v] << ' ';
  }
  template <class Vertex, class Graph> void
  finish_vertex(Vertex v, const Graph &)
  {
    std::cout << ' ' << name[v] << ")";
  }
};

int
main()
{
  using namespace boost;
  GraphvizGraph g;
  read_graphviz("figs/dfs-example.dot", g);
  graph_traits < GraphvizGraph >::edge_iterator e, e_end;
  for (boost::tie(e, e_end) = edges(g); e != e_end; ++e)
    std::cout << '(' << name[source(*e, g)] << ' '
      << name[target(*e, g)] << ')' << std::endl;
  parenthesis_visitor
    paren_vis;
  depth_first_search(g, visitor(paren_vis));
  std::cout << std::endl;
  return 0;
}
