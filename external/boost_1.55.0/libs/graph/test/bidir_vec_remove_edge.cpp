//  (C) Copyright 2004 Douglas Gregor and Jeremy Siek 
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// NOTE: This test illustrates a longstanding bug in the
// adjacency_list class template. We do not test it because it will
// cause problems until we have time to fix the bug. Annoying? Yes.

#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/test/minimal.hpp>

struct edge_prop {
  int weight;
};

int
test_main(int, char*[])
{
  {
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
      boost::no_property, edge_prop, boost::no_property, boost::vecS> graph;
    typedef boost::graph_traits<graph>::edge_descriptor edge;

    graph g(2);

    edge_prop p1 = { 42 };
    edge_prop p2 = { 17 };
    add_edge(0, 1, p1, g);
    add_edge(1, 0, p2, g);
    
    edge e1 = boost::edge(0, 1, g).first;
    edge e2 = boost::edge(1, 0, g).first;
    BOOST_CHECK( num_edges(g) == 2 );
    BOOST_CHECK( g[e1].weight == 42 );
    BOOST_CHECK( g[e2].weight == 17 );
    remove_edge(e1, g);
    BOOST_CHECK( num_edges(g) == 1 );

    // e2 has been invalidated, so grab it again
    bool b2;
    boost::tie(e2, b2) = boost::edge(1, 0, g);
    BOOST_CHECK( b2 );
    BOOST_CHECK( g[e2].weight == 17 );

    /* Now remove the other edge. Here, the fact that
     * stored_ra_edge_iterator keeps an index but does not update it
     * when edges are removed. So, this will be incorrect but the
     * error may not always show up (use an STL debug mode to see the
     * error for sure.)
     */
    remove_edge(e2, g);

  }
  return boost::exit_success;
}
