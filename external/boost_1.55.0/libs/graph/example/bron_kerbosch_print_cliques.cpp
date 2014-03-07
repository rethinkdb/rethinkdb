// (C) Copyright Andrew Sutton 2007
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

//[code_bron_kerbosch_print_cliques
#include <iostream>

#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/bron_kerbosch_all_cliques.hpp>

#include "helper.hpp"

using namespace std;
using namespace boost;

// The clique_printer is a visitor that will print the vertices that comprise
// a clique. Note that the vertices are not given in any specific order.
template <typename OutputStream>
struct clique_printer
{
    clique_printer(OutputStream& stream)
        : os(stream)
    { }

    template <typename Clique, typename Graph>
    void clique(const Clique& c, const Graph& g)
    {
        // Iterate over the clique and print each vertex within it.
        typename Clique::const_iterator i, end = c.end();
        for(i = c.begin(); i != end; ++i) {
            os << g[*i].name << " ";
        }
        os << endl;
    }
    OutputStream& os;
};

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

int
main(int argc, char *argv[])
{
    // Create the graph and and its name map accessor.
    Graph g;
    NameMap nm(get(&Actor::name, g));

    // Read the graph from standard input.
    read_graph(g, nm, cin);

    // Instantiate the visitor for printing cliques
    clique_printer<ostream> vis(cout);

    // Use the Bron-Kerbosch algorithm to find all cliques, printing them
    // as they are found.
    bron_kerbosch_all_cliques(g, vis);

    return 0;
}
//]
