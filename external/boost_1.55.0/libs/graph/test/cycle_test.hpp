// (C) Copyright 2013 Louis Dionne
//
// Modified from `tiernan_all_cycles.cpp`.
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_TEST_CYCLE_TEST_HPP
#define BOOST_GRAPH_TEST_CYCLE_TEST_HPP

#include <boost/assert.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/undirected_graph.hpp>
#include <boost/next_prior.hpp>
#include <boost/random/linear_congruential.hpp>
#include <cstddef>
#include <iostream>


namespace cycle_test_detail {
    using namespace boost;

    struct cycle_validator {
        explicit cycle_validator(std::size_t& number_of_cycles)
            : cycles(number_of_cycles)
        { }

        template <typename Path, typename Graph>
        void cycle(Path const& p, Graph const& g) {
            ++cycles;
            // Check to make sure that each of the vertices in the path
            // is truly connected and that the back is connected to the
            // front - it's not validating that we find all paths, just
            // that the paths are valid.
            typename Path::const_iterator i, j, last = prior(p.end());
            for (i = p.begin(); i != last; ++i) {
                j = boost::next(i);
                BOOST_ASSERT(edge(*i, *j, g).second);
            }
            BOOST_ASSERT(edge(p.back(), p.front(), g).second);
        }

        std::size_t& cycles;
    };

    template <typename Graph, typename Algorithm>
    void test_one(Algorithm algorithm) {
        typedef erdos_renyi_iterator<minstd_rand, Graph> er;

        // Generate random graphs with 15 vertices and 15% probability
        // of edge connection.
        static std::size_t const N = 20;
        static double const P = 0.1;
        minstd_rand rng;

        Graph g(er(rng, N, P), er(), N);
        renumber_indices(g);
        print_edges(g, get(vertex_index, g));

        std::size_t cycles = 0;
        cycle_validator vis(cycles);
        algorithm(g, vis);
        std::cout << "# cycles: " << vis.cycles << "\n";
    }
} // end namespace cycle_test_detail

template <typename Algorithm>
void cycle_test(Algorithm const& algorithm) {
    std::cout << "*** undirected ***\n";
    cycle_test_detail::test_one<boost::undirected_graph<> >(algorithm);

    std::cout << "*** directed ***\n";
    cycle_test_detail::test_one<boost::directed_graph<> >(algorithm);
}

#endif // !BOOST_GRAPH_TEST_CYCLE_TEST_HPP
