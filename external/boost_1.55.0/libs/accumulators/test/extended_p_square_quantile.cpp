//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for extended_p_square_quantile.hpp

#include <iostream>
#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/numeric/functional/complex.hpp>
#include <boost/accumulators/numeric/functional/valarray.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/extended_p_square_quantile.hpp>

using namespace boost;
using namespace unit_test;
using namespace boost::accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    typedef accumulator_set<double, stats<tag::extended_p_square_quantile> > accumulator_t;
    typedef accumulator_set<double, stats<tag::weighted_extended_p_square_quantile>, double > accumulator_t_weighted;
    typedef accumulator_set<double, stats<tag::extended_p_square_quantile(quadratic)> > accumulator_t_quadratic;
    typedef accumulator_set<double, stats<tag::weighted_extended_p_square_quantile(quadratic)>, double > accumulator_t_weighted_quadratic;

    // tolerance
    double epsilon = 1;

    // a random number generator
    boost::lagged_fibonacci607 rng;

    std::vector<double> probs;

    probs.push_back(0.990);
    probs.push_back(0.991);
    probs.push_back(0.992);
    probs.push_back(0.993);
    probs.push_back(0.994);
    probs.push_back(0.995);
    probs.push_back(0.996);
    probs.push_back(0.997);
    probs.push_back(0.998);
    probs.push_back(0.999);

    accumulator_t acc(extended_p_square_probabilities = probs);
    accumulator_t_weighted acc_weighted(extended_p_square_probabilities = probs);
    accumulator_t_quadratic acc2(extended_p_square_probabilities = probs);
    accumulator_t_weighted_quadratic acc_weighted2(extended_p_square_probabilities = probs);

    for (int i=0; i<10000; ++i)
    {
        double sample = rng();
        acc(sample);
        acc2(sample);
        acc_weighted(sample, weight = 1.);
        acc_weighted2(sample, weight = 1.);
    }

    for (std::size_t i = 0; i < probs.size() - 1; ++i)
    {
        BOOST_CHECK_CLOSE(
            quantile(acc, quantile_probability = 0.99025 + i*0.001)
          , 0.99025 + i*0.001
          , epsilon
        );
        BOOST_CHECK_CLOSE(
            quantile(acc2, quantile_probability = 0.99025 + i*0.001)
          , 0.99025 + i*0.001
          , epsilon
        );
        BOOST_CHECK_CLOSE(
            quantile(acc_weighted, quantile_probability = 0.99025 + i*0.001)
          , 0.99025 + i*0.001
          , epsilon
        );
        BOOST_CHECK_CLOSE(
            quantile(acc_weighted2, quantile_probability = 0.99025 + i*0.001)
          , 0.99025 + i*0.001
          , epsilon
        );
    }
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("extended_p_square_quantile test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

