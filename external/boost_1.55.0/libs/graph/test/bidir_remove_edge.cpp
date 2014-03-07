//  (C) Copyright Jeremy Siek 2004 
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/cstdlib.hpp>
#include <boost/detail/lightweight_test.hpp>

struct edge_prop {
  int weight;
};

int
main(int, char*[])
{
  {
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
      boost::no_property, edge_prop> graph;
    typedef boost::graph_traits<graph>::edge_descriptor edge;

    graph g(2);

    edge_prop p = { 42 };
    edge e; bool b;
    boost::tie(e, b) = add_edge(0, 1, p, g);
    BOOST_TEST( num_edges(g) == 1 );
    BOOST_TEST( g[e].weight == 42 );
    remove_edge(e, g);
    BOOST_TEST( num_edges(g) == 0 );
  }
  {
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS> graph;
    typedef boost::graph_traits<graph>::edge_descriptor edge;

    graph g(2);

    edge e; bool b;
    boost::tie(e, b) = add_edge(0, 1, g);
    BOOST_TEST( num_edges(g) == 1 );
    remove_edge(e, g);
    BOOST_TEST( num_edges(g) == 0 );
  }
  return boost::report_errors();
}
