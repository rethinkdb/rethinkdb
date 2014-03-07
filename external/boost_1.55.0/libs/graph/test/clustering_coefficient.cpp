// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/clustering_coefficient.hpp>

using namespace std;
using namespace boost;

// number of vertices in the graph
static const unsigned N = 5;

template <typename Graph>
struct vertex_vector
{
    typedef graph_traits<Graph> traits;
    typedef vector<typename traits::vertex_descriptor> type;
};

template <typename Graph>
void build_graph(Graph& g, typename vertex_vector<Graph>::type& v)
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

    typedef exterior_vertex_property<Graph, double> ClusteringProperty;
    typedef typename ClusteringProperty::container_type ClusteringContainer;
    typedef typename ClusteringProperty::map_type ClusteringMap;

    Graph g;
    vector<Vertex> v(N);
    build_graph(g, v);

    ClusteringContainer cc(num_vertices(g));
    ClusteringMap cm(cc, g);

    BOOST_ASSERT(num_paths_through_vertex(g, v[0]) == 3);
    BOOST_ASSERT(num_paths_through_vertex(g, v[1]) == 1);
    BOOST_ASSERT(num_paths_through_vertex(g, v[2]) == 1);
    BOOST_ASSERT(num_paths_through_vertex(g, v[3]) == 0);
    BOOST_ASSERT(num_paths_through_vertex(g, v[4]) == 1);

    BOOST_ASSERT(num_triangles_on_vertex(g, v[0]) == 1);
    BOOST_ASSERT(num_triangles_on_vertex(g, v[1]) == 1);
    BOOST_ASSERT(num_triangles_on_vertex(g, v[2]) == 1);
    BOOST_ASSERT(num_triangles_on_vertex(g, v[3]) == 0);
    BOOST_ASSERT(num_triangles_on_vertex(g, v[4]) == 0);

    // TODO: Need a FP approximation to assert here.
    // BOOST_ASSERT(clustering_coefficient(g, v[0]) == double(1)/3);
    BOOST_ASSERT(clustering_coefficient(g, v[1]) == 1);
    BOOST_ASSERT(clustering_coefficient(g, v[2]) == 1);
    BOOST_ASSERT(clustering_coefficient(g, v[3]) == 0);
    BOOST_ASSERT(clustering_coefficient(g, v[4]) == 0);

    all_clustering_coefficients(g, cm);

    // TODO: Need a FP approximation to assert here.
    // BOOST_ASSERT(cm[v[0]] == double(1)/3);
    BOOST_ASSERT(cm[v[1]] == 1);
    BOOST_ASSERT(cm[v[2]] == 1);
    BOOST_ASSERT(cm[v[3]] == 0);
    BOOST_ASSERT(cm[v[4]] == 0);

    // I would have used check_close, but apparently, that requires
    // me to link this against a library - which I don't really want
    // to do. Basically, this makes sure that that coefficient is
    // within some tolerance (like 1/10 million).
    double coef = mean_clustering_coefficient(g, cm);
    BOOST_ASSERT((coef - (7.0f / 15.0f)) < 1e-7f);
}

int
main(int, char *[])
{
    typedef undirected_graph<> Graph;
    // typedef directed_graph<> Digraph;

    // TODO: write a test for directed clustering coefficient.

    test_undirected<Graph>();
    // test<Digraph>();
}
