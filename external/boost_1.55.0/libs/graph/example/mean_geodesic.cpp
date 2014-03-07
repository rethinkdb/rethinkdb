// (C) Copyright Andrew Sutton 2007
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

//[mean_geodesic_example
#include <iostream>
#include <iomanip>

#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/geodesic_distance.hpp>
#include "helper.hpp"

using namespace std;
using namespace boost;

// The Actor type stores the name of each vertex in the graph.
struct Actor
{
    string name;
};

// Declare the graph type and its vertex and edge types.
typedef undirected_graph<Actor> Graph;
typedef graph_traits<Graph>::vertex_descriptor Vertex;
typedef graph_traits<Graph>::edge_descriptor Edge;

// The name map provides an abstract accessor for the names of
// each vertex. This is used during graph creation.
typedef property_map<Graph, string Actor::*>::type NameMap;

// Declare a matrix type and its corresponding property map that
// will contain the distances between each pair of vertices.
typedef exterior_vertex_property<Graph, int> DistanceProperty;
typedef DistanceProperty::matrix_type DistanceMatrix;
typedef DistanceProperty::matrix_map_type DistanceMatrixMap;

// Declare the weight map so that each edge returns the same value.
typedef constant_property_map<Edge, int> WeightMap;

// Declare a container and its corresponding property map that
// will contain the resulting mean geodesic distances of each
// vertex in the graph.
typedef exterior_vertex_property<Graph, float> GeodesicProperty;
typedef GeodesicProperty::container_type GeodesicContainer;
typedef GeodesicProperty::map_type GeodesicMap;

int
main(int argc, char *argv[])
{
    // Create the graph and a property map that provides access
    // to the actor names.
    Graph g;
    NameMap nm(get(&Actor::name, g));

    // Read the graph from standad input.
    read_graph(g, nm, cin);

    // Compute the distances between all pairs of vertices using
    // the Floyd-Warshall algorithm. Note that the weight map is
    // created so that every edge has a weight of 1.
    DistanceMatrix distances(num_vertices(g));
    DistanceMatrixMap dm(distances, g);
    WeightMap wm(1);
    floyd_warshall_all_pairs_shortest_paths(g, dm, weight_map(wm));

    // Compute the mean geodesic distances for each vertex in
    // the graph and get the average mean geodesic distace (the
    // so-called small-world distance) as a result.
    GeodesicContainer geodesics(num_vertices(g));
    GeodesicMap gm(geodesics, g);
    float sw = all_mean_geodesics(g, dm, gm);

    // Print the mean geodesic distance of each vertex and finally,
    // the graph itself.
    graph_traits<Graph>::vertex_iterator i, end;
    for(boost::tie(i, end) = vertices(g); i != end; ++i) {
        cout << setw(12) << setiosflags(ios::left)
             << g[*i].name << get(gm, *i) << endl;
    }
    cout << "small world distance: " << sw << endl;


    return 0;
}
//]
