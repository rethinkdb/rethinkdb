// (C) Copyright Andrew Sutton 2007
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

//[scaled_closeness_centrality_example
#include <iostream>
#include <iomanip>

#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/closeness_centrality.hpp>
#include "helper.hpp"

using namespace std;
using namespace boost;

// This template struct provides a generic version of a "scaling"
// closeness measure. Specifically, this implementation divides
// the number of vertices in the graph by the sum of geodesic
// distances of each vertex. This measure allows customization
// of the distance type, result type, and even the underlying
// divide operation.
template <typename Graph,
          typename Distance,
          typename Result,
          typename Divide = divides<Result> >
struct scaled_closeness_measure
{
    typedef Distance distance_type;
    typedef Result result_type;

    Result operator ()(Distance d, const Graph& g)
    {
        if(d == numeric_values<Distance>::infinity()) {
            return numeric_values<Result>::zero();
        }
        else {
            return div(Result(num_vertices(g)), Result(d));
        }
    }
    Divide div;
};

// The Actor type stores the name of each vertex in the graph.
struct Actor
{
    std::string name;
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
// will contain the resulting closeness centralities of each
// vertex in the graph.
typedef boost::exterior_vertex_property<Graph, float> ClosenessProperty;
typedef ClosenessProperty::container_type ClosenessContainer;
typedef ClosenessProperty::map_type ClosenessMap;

int
main(int argc, char *argv[])
{
    // Create the graph and a property map that provides access
    // to the actor names.
    Graph g;
    NameMap nm(get(&Actor::name, g));

    // Read the graph from standard input.
    read_graph(g, nm, cin);

    // Compute the distances between all pairs of vertices using
    // the Floyd-Warshall algorithm. Note that the weight map is
    // created so that every edge has a weight of 1.
    DistanceMatrix distances(num_vertices(g));
    DistanceMatrixMap dm(distances, g);
    WeightMap wm(1);
    floyd_warshall_all_pairs_shortest_paths(g, dm, weight_map(wm));

    // Create the scaled closeness measure.
    scaled_closeness_measure<Graph, int, float> m;

    // Compute the degree centrality for graph
    ClosenessContainer cents(num_vertices(g));
    ClosenessMap cm(cents, g);
    all_closeness_centralities(g, dm, cm, m);

    // Print the scaled closeness centrality of each vertex.
    graph_traits<Graph>::vertex_iterator i, end;
    for(boost::tie(i, end) = vertices(g); i != end; ++i) {
        cout << setw(12) << setiosflags(ios::left)
             << g[*i].name << get(cm, *i) << endl;
    }

    return 0;
}
//]
