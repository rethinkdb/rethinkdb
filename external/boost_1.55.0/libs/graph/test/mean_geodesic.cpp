// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/property_maps/constant_property_map.hpp>

#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/geodesic_distance.hpp>

using namespace std;
using namespace boost;

// useful types
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
    typedef typename graph_traits<Graph>::edge_descriptor Edge;

    typedef exterior_vertex_property<Graph, double> CentralityProperty;
    typedef typename CentralityProperty::container_type CentralityContainer;
    typedef typename CentralityProperty::map_type CentralityMap;

    typedef exterior_vertex_property<Graph, int> DistanceProperty;
    typedef typename DistanceProperty::matrix_type DistanceMatrix;
    typedef typename DistanceProperty::matrix_map_type DistanceMatrixMap;

    typedef constant_property_map<Edge, int> WeightMap;

    Graph g;
    vector<Vertex> v(N);
    build_graph(g, v);

    CentralityContainer centralities(num_vertices(g));
    DistanceMatrix distances(num_vertices(g));

    CentralityMap cm(centralities, g);
    DistanceMatrixMap dm(distances, g);

    WeightMap wm(1);

    floyd_warshall_all_pairs_shortest_paths(g, dm, weight_map(wm));
    double geo1 = all_mean_geodesics(g, dm, cm);
    double geo2 = small_world_distance(g, cm);

    BOOST_ASSERT(cm[v[0]] == double(5)/4);
    BOOST_ASSERT(cm[v[1]] == double(7)/4);
    BOOST_ASSERT(cm[v[2]] == double(7)/4);
    BOOST_ASSERT(cm[v[3]] == double(9)/4);
    BOOST_ASSERT(cm[v[4]] == double(6)/4);
    BOOST_ASSERT(geo1 == double(34)/20);
    BOOST_ASSERT(geo1 == geo2);
}

template <typename Graph>
void test_directed()
{
    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
    typedef typename graph_traits<Graph>::edge_descriptor Edge;

    typedef exterior_vertex_property<Graph, double> CentralityProperty;
    typedef typename CentralityProperty::container_type CentralityContainer;
    typedef typename CentralityProperty::map_type CentralityMap;

    typedef exterior_vertex_property<Graph, int> DistanceProperty;
    typedef typename DistanceProperty::matrix_type DistanceMatrix;
    typedef typename DistanceProperty::matrix_map_type DistanceMatrixMap;

    typedef constant_property_map<Edge, int> WeightMap;

    Graph g;
    vector<Vertex> v(N);
    build_graph(g, v);

    CentralityContainer centralities(num_vertices(g));
    DistanceMatrix distances(num_vertices(g));

    CentralityMap cm(centralities, g);
    DistanceMatrixMap dm(distances, g);

    WeightMap wm(1);

    floyd_warshall_all_pairs_shortest_paths(g, dm, weight_map(wm));
    double geo1 = all_mean_geodesics(g, dm, cm);
    double geo2 = small_world_distance(g, cm);

    double inf = numeric_values<double>::infinity();
    BOOST_ASSERT(cm[v[0]] == inf);
    BOOST_ASSERT(cm[v[1]] == inf);
    BOOST_ASSERT(cm[v[2]] == inf);
    BOOST_ASSERT(cm[v[3]] == double(10)/4);
    BOOST_ASSERT(cm[v[4]] == inf);
    BOOST_ASSERT(geo1 == inf);
    BOOST_ASSERT(geo1 == geo2);
}


int
main(int, char *[])
{
    typedef undirected_graph<> Graph;
    typedef directed_graph<> Digraph;

    test_undirected<Graph>();
    test_directed<Digraph>();
}
