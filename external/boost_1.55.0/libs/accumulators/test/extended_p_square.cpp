//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for extended_p_square.hpp

#include <iostream>
#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/numeric/functional/complex.hpp>
#include <boost/accumulators/numeric/functional/valarray.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/extended_p_square.hpp>

using namespace boost;
using namespace unit_test;
using namespace boost::accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    typedef accumulator_set<double, stats<tag::extended_p_square> > accumulator_t;

    // tolerance
    double epsilon = 3;

    // a random number generator
    boost::lagged_fibonacci607 rng;

    std::vector<double> probs;

    probs.push_back(0.001);
    probs.push_back(0.01 );
    probs.push_back(0.1  );
    probs.push_back(0.25 );
    probs.push_back(0.5  );
    probs.push_back(0.75 );
    probs.push_back(0.9  );
    probs.push_back(0.99 );
    probs.push_back(0.999);

    accumulator_t acc(extended_p_square_probabilities = probs);

    for (int i=0; i<10000; ++i)
        acc(rng());

    BOOST_CHECK_GE(extended_p_square(acc)[0], 0.0005);
    BOOST_CHECK_LE(extended_p_square(acc)[0], 0.0015);
    BOOST_CHECK_CLOSE(extended_p_square(acc)[1], probs[1], 15);
    BOOST_CHECK_CLOSE(extended_p_square(acc)[2], probs[2], 5);

    for (std::size_t i=3; i<probs.size(); ++i)
    {
        BOOST_CHECK_CLOSE(extended_p_square(acc)[i], probs[i], epsilon);
    }
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("extended_p_square test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

