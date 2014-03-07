// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/directed_graph.hpp>

using namespace std;
using namespace boost;

template <typename Graph>
void test()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
    typedef typename property_map<Graph, vertex_index_t>::type IndexMap;

    static const size_t N = 5;

    Graph g;
    (void)(IndexMap)get(vertex_index, g);

    // build up the graph
    Vertex v[N];
    for(size_t i = 0; i < N; ++i) {
        v[i] = add_vertex(g);
    }

    // after the first build, we should have these conditions
    BOOST_ASSERT(max_vertex_index(g) == N);
    for(size_t i = 0; i < N; ++i) {
        BOOST_ASSERT(get_vertex_index(v[i], g) == i);
    }

    // Remove all vertices and then re-add them.
    for(size_t i = 0; i < N; ++i) remove_vertex(v[i], g);
    BOOST_ASSERT(num_vertices(g) == 0);
    for(size_t i = 0; i < N; ++i) {
        v[i] = add_vertex(g);
    }
    BOOST_ASSERT(num_vertices(g) == N);

    // Before renumbering, our vertices should be off by N. In other words,
    // we're not in a good state.
    BOOST_ASSERT(max_vertex_index(g) == 10);
    for(size_t i = 0; i < N; ++i) {
        BOOST_ASSERT(get_vertex_index(v[i], g) == N + i);
    }

    // Renumber vertices
    renumber_vertex_indices(g);

    // And we should be back to the initial condition
    BOOST_ASSERT(max_vertex_index(g) == N);
    for(size_t i = 0; i < N; ++i) {
        BOOST_ASSERT(get_vertex_index(v[i], g) == i);
    }
}

// Make sure that graphs constructed over n vertices will actually number
// their vertices correctly.
template <typename Graph>
void build()
{
    typedef typename graph_traits<Graph>::vertex_iterator Iterator;
    typedef typename property_map<Graph, vertex_index_t>::type IndexMap;

    static const size_t N = 5;

    Graph g(N);
    BOOST_ASSERT(max_vertex_index(g) == N);

    (void)(IndexMap)get(vertex_index, g);

    // Each vertex should be numbered correctly.
    Iterator i, end;
    boost::tie(i, end) = vertices(g);
    for(size_t x = 0; i != end; ++i, ++x) {
        BOOST_ASSERT(get_vertex_index(*i, g) == x);
    }
}

int main(int, char*[])
{
    test< undirected_graph<> >();
    test< directed_graph<> >();

    build< undirected_graph<> >();

    return 0;
}
