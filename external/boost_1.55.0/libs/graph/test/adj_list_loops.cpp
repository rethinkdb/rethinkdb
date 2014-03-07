// (C) Copyright Andrew Sutton 2009
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/assert.hpp>
#include <boost/graph/adjacency_list.hpp>

using namespace boost;

// TODO: Integrate this into a larger adj_list test suite.

template <typename Graph>
void test_graph_nonloop()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;

    // Build a graph with 1 edge and turn it into a loop.
    Graph g(5);
    Vertex u = *vertices(g).first;
    Vertex v = *next(vertices(g).first, 2);
    add_edge(u, v, g);
    BOOST_ASSERT(num_vertices(g) == 5);
    BOOST_ASSERT(num_edges(g) == 1);
    remove_edge(u, v, g);
    BOOST_ASSERT(num_edges(g) == 0);

}


template <typename Graph>
void test_multigraph_nonloop()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;

    // Build a graph with 1 edge and turn it into a loop.
    Graph g(5);
    Vertex u = *vertices(g).first;
    Vertex v = *next(vertices(g).first, 2);
    add_edge(u, v, g);
    add_edge(u, v, g);
    BOOST_ASSERT(num_vertices(g) == 5);
    BOOST_ASSERT(num_edges(g) == 2);
    remove_edge(u, v, g);
    BOOST_ASSERT(num_edges(g) == 0);

}

template <typename Graph>
void test_graph_loop()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;

    Graph g(5);
    Vertex v = *next(vertices(g).first, 2);
    add_edge(v, v, g);
    BOOST_ASSERT(num_vertices(g) == 5);
    BOOST_ASSERT(num_edges(g) == 1);
    remove_edge(v, v, g);
    BOOST_ASSERT(num_edges(g) == 0);
}

template <typename Graph>
void test_multigraph_loop()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;

    Graph g(5);
    Vertex v = *next(vertices(g).first, 2);
    add_edge(v, v, g);
    add_edge(v, v, g);
    BOOST_ASSERT(num_vertices(g) == 5);
    BOOST_ASSERT(num_edges(g) == 2);
    remove_edge(v, v, g);
    BOOST_ASSERT(num_edges(g) == 0);
}

template <typename Kind>
void test()
{
    typedef no_property na;
    typedef adjacency_list<vecS, vecS, Kind, na, na, na, listS> VVL;
    typedef adjacency_list<listS, vecS, Kind, na, na, na, listS> LVL;
    typedef adjacency_list<setS, vecS, Kind, na, na, na, listS> SVL;
    typedef adjacency_list<multisetS, vecS, Kind, na, na, na, listS> MVL;

    test_graph_nonloop<VVL>();
    test_graph_nonloop<LVL>();
    test_graph_nonloop<SVL>();
    test_graph_nonloop<MVL>();
    test_multigraph_nonloop<VVL>();
    test_multigraph_nonloop<LVL>();
    test_multigraph_nonloop<MVL>();
    test_graph_loop<VVL>();
    test_graph_loop<LVL>();
    test_graph_loop<SVL>();
    test_graph_loop<MVL>();
    test_multigraph_loop<VVL>();
    test_multigraph_loop<LVL>();
    test_multigraph_loop<MVL>();
}


int main()
{
    test<undirectedS>();
    test<directedS>();
    test<bidirectionalS>();

    return 0;
}
