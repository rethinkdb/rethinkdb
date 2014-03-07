//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for p_square_quantile.hpp

#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/numeric/functional/complex.hpp>
#include <boost/accumulators/numeric/functional/valarray.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/p_square_quantile.hpp>

using namespace boost;
using namespace unit_test;
using namespace boost::accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    typedef accumulator_set<double, stats<tag::p_square_quantile> > accumulator_t;

    // tolerance in %
    double epsilon = 1;

    // a random number generator
    boost::lagged_fibonacci607 rng;

    accumulator_t acc0(quantile_probability = 0.001);
    accumulator_t acc1(quantile_probability = 0.01 );
    accumulator_t acc2(quantile_probability = 0.1  );
    accumulator_t acc3(quantile_probability = 0.25 );
    accumulator_t acc4(quantile_probability = 0.5  );
    accumulator_t acc5(quantile_probability = 0.75 );
    accumulator_t acc6(quantile_probability = 0.9  );
    accumulator_t acc7(quantile_probability = 0.99 );
    accumulator_t acc8(quantile_probability = 0.999);

    for (int i=0; i<100000; ++i)
    {
        double sample = rng();
        acc0(sample);
        acc1(sample);
        acc2(sample);
        acc3(sample);
        acc4(sample);
        acc5(sample);
        acc6(sample);
        acc7(sample);
        acc8(sample);
    }

    BOOST_CHECK_CLOSE( p_square_quantile(acc0), 0.001, 18*epsilon );
    BOOST_CHECK_CLOSE( p_square_quantile(acc1), 0.01 , 7*epsilon );
    BOOST_CHECK_CLOSE( p_square_quantile(acc2), 0.1  , 3*epsilon );
    BOOST_CHECK_CLOSE( p_square_quantile(acc3), 0.25 , 2*epsilon );
    BOOST_CHECK_CLOSE( p_square_quantile(acc4), 0.5  , epsilon );
    BOOST_CHECK_CLOSE( p_square_quantile(acc5), 0.75 , epsilon );
    BOOST_CHECK_CLOSE( p_square_quantile(acc6), 0.9  , epsilon );
    BOOST_CHECK_CLOSE( p_square_quantile(acc7), 0.99 , epsilon );
    BOOST_CHECK_CLOSE( p_square_quantile(acc8), 0.999, epsilon );
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("p_square_quantile test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

