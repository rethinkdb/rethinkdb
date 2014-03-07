// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iterator>
#include <algorithm>
#include <vector>
#include <map>

#include <boost/graph/graph_utility.hpp>
#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/bron_kerbosch_all_cliques.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>

#include <boost/random/linear_congruential.hpp>

using namespace std;
using namespace boost;

// TODO: This is probably not a very good test. We should be able to define
// an identity test - something like find the clique of K5.

struct clique_validator
{
    clique_validator()
    { }

    template <typename Clique, typename Graph>
    inline void
    clique(const Clique& c, Graph& g)
    {
        // Simply assert that each vertex in the clique is connected
        // to all others in the clique.
        typename Clique::const_iterator i, j, end = c.end();
        for(i = c.begin(); i != end; ++i) {
            for(j = c.begin(); j != end; ++j) {
                if(i != j) {
                    BOOST_ASSERT(edge(*i, *j, g).second);
                }
            }
        }
    }
};

template <typename Graph>
void test()
{
    typedef erdos_renyi_iterator<boost::minstd_rand, Graph> er;

    // Generate random graphs with 15 vertices and 15% probability
    // of edge connection.
    static const size_t N = 20;
    static const double P = 0.1;

    boost::minstd_rand rng;
    Graph g(er(rng, N, P), er(), N);
    renumber_indices(g);
    print_edges(g, get(vertex_index, g));

    bron_kerbosch_all_cliques(g, clique_validator());
}

int
main(int, char *[])
{
    typedef undirected_graph<> Graph;
    typedef directed_graph<> DiGraph;

    std::cout << "*** undirected ***\n";
    test<Graph>();

    std::cout << "*** directed ***\n";
    test<DiGraph>();
}
