// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/graph/graph_utility.hpp>
#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/tiernan_all_cycles.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>

#include <boost/random/linear_congruential.hpp>

using namespace std;
using namespace boost;

struct cycle_validator
{
    cycle_validator(size_t& c)
        : cycles(c)
    { }

    template <typename Path, typename Graph>
    void cycle(const Path& p, const Graph& g)
    {
        ++cycles;
        // Check to make sure that each of the vertices in the path
        // is truly connected and that the back is connected to the
        // front - it's not validating that we find all paths, just
        // that the paths are valid.
        typename Path::const_iterator i, j, last = prior(p.end());
        for(i = p.begin(); i != last; ++i) {
            j = boost::next(i);
            BOOST_ASSERT(edge(*i, *j, g).second);
        }
        BOOST_ASSERT(edge(p.back(), p.front(), g).second);
    }

    size_t& cycles;
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

    size_t cycles = 0;
    cycle_validator vis(cycles);
    tiernan_all_cycles(g, vis);
    cout << "# cycles: " << vis.cycles << "\n";
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
