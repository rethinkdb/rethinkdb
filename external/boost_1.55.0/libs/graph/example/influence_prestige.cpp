// (C) Copyright Andrew Sutton 2007
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

//[influence_prestige_example
#include <iostream>
#include <iomanip>

#include <boost/graph/directed_graph.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/degree_centrality.hpp>

#include "helper.hpp"

using namespace std;
using namespace boost;

// The Actor type stores the name of each vertex in the graph.
struct Actor
{
    string name;
};

// Declare the graph type and its vertex and edge types.
typedef directed_graph<Actor> Graph;
typedef graph_traits<Graph>::vertex_descriptor Vertex;
typedef graph_traits<Graph>::edge_descriptor Edge;

// The name map provides an abstract accessor for the names of
// each vertex. This is used during graph creation.
typedef property_map<Graph, string Actor::*>::type NameMap;

// Declare a container type for influence and prestige (both
// of which are degree centralities) and its corresponding
// property map.
typedef exterior_vertex_property<Graph, unsigned> CentralityProperty;
typedef CentralityProperty::container_type CentralityContainer;
typedef CentralityProperty::map_type CentralityMap;

int
main(int argc, char *argv[])
{
    // Create the graph and a property map that provides
    // access to the actor names.
    Graph g;
    NameMap nm(get(&Actor::name, g));

    // Read the graph from standard input.
    read_graph(g, nm, cin);

    // Compute the influence for the graph.
    CentralityContainer influence(num_vertices(g));
    CentralityMap im(influence, g);
    all_influence_values(g, im);

    // Compute the influence for the graph.
    CentralityContainer prestige(num_vertices(g));
    CentralityMap pm(prestige, g);
    all_prestige_values(g, pm);

    // Print the degree centrality of each vertex
    graph_traits<Graph>::vertex_iterator i, end;
    for(boost::tie(i, end) = vertices(g); i != end; ++i) {
        Vertex v = *i;
        cout << setiosflags(ios::left) << setw(12)
             << g[v].name << "\t"
             << im[v] << "\t"
             << pm[v] << endl;
    }

    return 0;
}
//]
