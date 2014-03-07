// (C) Copyright Andrew Sutton 2009
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <boost/graph/adjacency_list.hpp>

#include "../../../../../gpld/common/typestr.hpp"

using namespace std;
using namespace boost;

// The purpose of this test is simply to provide a testing ground for the
// invalidation of iterators and descriptors.

template <typename Graph>
void make_graph(Graph& g)
{
    // Build a simple (barbell) graph.
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
    Vertex u = add_vertex(10, g);
    Vertex v = add_vertex(20, g);
    add_edge(u, v, 100, g);
}

// Invalid iterators and descriptors will cause a segfault.
template <typename Graph, typename Iterator, typename Descriptor>
void test(Graph& g, Iterator i, Descriptor d, string const& str)
{
    int x;
    cout << "... " << str << " iter" << endl;
    x = g[*i];
//     cout << "... " << x << endl;
    cout << "... " << str << " desc" << endl;
    x = g[d];
//     cout << "... " << x << endl;
}

template <typename Graph>
void invalidate_edges()
{
    typedef typename graph_traits<Graph>::edge_descriptor Edge;
    typedef typename graph_traits<Graph>::edge_iterator EdgeIterator;

    Graph g;
    make_graph(g);

    // The actual test. These are valid here.
    EdgeIterator i = edges(g).first;
    Edge e = *i;

    // Add a vertex, see what breaks.
    add_vertex(g);
    test(g, i, e, "edges");
};

template <typename Graph>
void invalidate_vertices()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
    typedef typename graph_traits<Graph>::vertex_iterator VertexIterator;

    Graph g;
    make_graph(g);

    // The actual test. These are valid here.
    VertexIterator i = vertices(g).first;
    Vertex v = *i;

    // Add a vertex, see what breaks.
    add_vertex(g);
    test(g, i, v, "vertices");
}

template <typename Graph>
void invalidate_out_edges()
{
    typedef typename graph_traits<Graph>::edge_descriptor Edge;
    typedef typename graph_traits<Graph>::out_edge_iterator OutIterator;

    Graph g;
    make_graph(g);

    // The actual test. These are valid here.
    OutIterator i = out_edges(*vertices(g).first, g).first;
    Edge e = *i;

    // Add a vertex, see what breaks.
    add_vertex(g);
    test(g, i, e, "out edges");
}

template <typename Graph>
void invalidate_adj_verts()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
    typedef typename graph_traits<Graph>::adjacency_iterator AdjIterator;

    Graph g;
    make_graph(g);

    // The actual test. These are valid here.
    AdjIterator i = adjacent_vertices(*vertices(g).first, g).first;
    Vertex v = *i;

    // Add a vertex, see what breaks.
    add_vertex(g);
    test(g, i, v, "adjacent vertices");
}


int main()
{
    typedef adjacency_list<vecS, vecS, undirectedS, int, int> VVU;
    cout << "vecS vecS undirectedS" << endl;
    invalidate_vertices<VVU>();
    invalidate_edges<VVU>();
    invalidate_out_edges<VVU>();
    invalidate_adj_verts<VVU>();

    typedef adjacency_list<vecS, vecS, bidirectionalS, int, int> VVB;
    cout << "vecS vecS bidirectionals" << endl;
    invalidate_vertices<VVB>();
    invalidate_edges<VVB>();
    invalidate_out_edges<VVB>();
    invalidate_adj_verts<VVB>();

    // If you comment out the tests before this, then adj_verts test will
    // run without segfaulting - at least under gcc-4.3. Not really sure why,
    // but I'm guessing it's still not generating valid results, and shouldn't
    // be taken as an indicator of stability.
    typedef adjacency_list<vecS, vecS, directedS, int, int> VVD;
    cout << "vecS vecS directedS" << endl;
    invalidate_vertices<VVD>();
//     invalidate_edges<VVD>();
//     invalidate_out_edges<VVD>();
//     invalidate_adj_verts<VVD>();

    typedef adjacency_list<listS, vecS, directedS, int, int> LVD;
    cout << "listS vecS directedS" << endl;
    invalidate_vertices<LVD>();
//     invalidate_edges<LVD>();
//     invalidate_out_edges<LVD>();
//     invalidate_adj_verts<LVD>();
}

