// (C) Copyright Andrew Sutton 2007
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)


//[code_clustering_coefficient
#include <iostream>
#include <iomanip>

#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/clustering_coefficient.hpp>
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

// The clustering property, container, and map define the containment
// and abstract accessor for the clustering coefficients of vertices.
typedef exterior_vertex_property<Graph, float> ClusteringProperty;
typedef ClusteringProperty::container_type ClusteringContainer;
typedef ClusteringProperty::map_type ClusteringMap;

int
main(int argc, char *argv[])
{
    // Create the graph and a name map that provides access to
    // then actor names.
    Graph g;
    NameMap nm(get(&Actor::name, g));

    // Read the graph from standard input.
    read_graph(g, nm, cin);

    // Compute the clustering coefficients of each vertex in the graph
    // and the mean clustering coefficient which is returned from the
    // computation.
    ClusteringContainer coefs(num_vertices(g));
    ClusteringMap cm(coefs, g);
    float cc = all_clustering_coefficients(g, cm);

    // Print the clustering coefficient of each vertex.
    graph_traits<Graph>::vertex_iterator i, end;
    for(boost::tie(i, end) = vertices(g); i != end; ++i) {
        cout << setw(12) << setiosflags(ios::left)
                << g[*i].name << get(cm, *i) << endl;
    }
    cout << "mean clustering coefficient: " << cc << endl;

    return 0;
}
//]
