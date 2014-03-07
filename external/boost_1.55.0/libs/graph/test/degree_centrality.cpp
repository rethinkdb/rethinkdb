// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include <vector>

#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/degree_centrality.hpp>
#include <boost/graph/exterior_property.hpp>

using namespace std;
using namespace boost;

// useful types
// number of vertices in the graph
static const unsigned N = 5;

template <typename Graph>
void build_graph(Graph& g,
                 vector<typename graph_traits<Graph>::vertex_descriptor>& v)
{
    // add vertices
    for(size_t i = 0; i < N; ++i) {
        v[i] = add_vertex(g);
    }

    // add edges
    add_edge(v[0], v[1], g);
    add_edge(v[1], v[2], g);
    add_edge(v[2], v[0], g);
    add_edge(v[3], v[4], g);
    add_edge(v[4], v[0], g);
}

template <typename Graph>
void test_undirected()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;

    typedef exterior_vertex_property<Graph, unsigned> CentralityProperty;
    typedef typename CentralityProperty::container_type CentralityContainer;
    typedef typename CentralityProperty::map_type CentralityMap;

    Graph g;
    vector<Vertex> v(N);
    build_graph(g, v);

    CentralityContainer cents(num_vertices(g));
    CentralityMap cm(cents, g);
    all_degree_centralities(g, cm);

    BOOST_ASSERT(cm[v[0]] == 3);
    BOOST_ASSERT(cm[v[1]] == 2);
    BOOST_ASSERT(cm[v[2]] == 2);
    BOOST_ASSERT(cm[v[3]] == 1);
    BOOST_ASSERT(cm[v[4]] == 2);
}

template <typename Graph>
void test_influence()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;

    typedef exterior_vertex_property<Graph, unsigned> CentralityProperty;
    typedef typename CentralityProperty::container_type CentralityContainer;
    typedef typename CentralityProperty::map_type CentralityMap;

    Graph g;

    vector<Vertex> v(N);
    build_graph(g, v);

    CentralityContainer cents(num_vertices(g));
    CentralityMap cm(cents, g);
    all_influence_values(g, cm);

    BOOST_ASSERT(cm[v[0]] == 1);
    BOOST_ASSERT(cm[v[1]] == 1);
    BOOST_ASSERT(cm[v[2]] == 1);
    BOOST_ASSERT(cm[v[3]] == 1);
    BOOST_ASSERT(cm[v[4]] == 1);
}

template <typename Graph>
void test_prestige()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;

    typedef exterior_vertex_property<Graph, unsigned> CentralityProperty;
    typedef typename CentralityProperty::container_type CentralityContainer;
    typedef typename CentralityProperty::map_type CentralityMap;

    Graph g;

    vector<Vertex> v(N);
    build_graph(g, v);

    CentralityContainer cents(num_vertices(g));
    CentralityMap cm(cents, g);
    all_prestige_values(g, cm);

    BOOST_ASSERT(cm[v[0]] == 2);
    BOOST_ASSERT(cm[v[1]] == 1);
    BOOST_ASSERT(cm[v[2]] == 1);
    BOOST_ASSERT(cm[v[3]] == 0);
    BOOST_ASSERT(cm[v[4]] == 1);
}

int
main(int, char *[])
{
    typedef undirected_graph<> Graph;
    typedef directed_graph<> Digraph;

    test_undirected<Graph>();
    test_influence<Digraph>();
    test_prestige<Digraph>();
}
