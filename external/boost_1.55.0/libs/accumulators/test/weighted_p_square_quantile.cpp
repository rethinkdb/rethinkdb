//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for weighted_p_square_quantile.hpp

#include <cmath> // for std::exp()
#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/numeric/functional/complex.hpp>
#include <boost/accumulators/numeric/functional/valarray.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_p_square_quantile.hpp>

using namespace boost;
using namespace unit_test;
using namespace boost::accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    typedef accumulator_set<double, stats<tag::weighted_p_square_quantile>, double> accumulator_t;

    // tolerance in %
    double epsilon = 1;

    // some random number generators
    double mu4 = -1.0;
    double mu5 = -1.0;
    double mu6 = 1.0;
    double mu7 = 1.0;
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma4(mu4, 1);
    boost::normal_distribution<> mean_sigma5(mu5, 1);
    boost::normal_distribution<> mean_sigma6(mu6, 1);
    boost::normal_distribution<> mean_sigma7(mu7, 1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal4(rng, mean_sigma4);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal5(rng, mean_sigma5);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal6(rng, mean_sigma6);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal7(rng, mean_sigma7);

    accumulator_t acc0(quantile_probability = 0.001);
    accumulator_t acc1(quantile_probability = 0.025);
    accumulator_t acc2(quantile_probability = 0.975);
    accumulator_t acc3(quantile_probability = 0.999);

    accumulator_t acc4(quantile_probability = 0.001);
    accumulator_t acc5(quantile_probability = 0.025);
    accumulator_t acc6(quantile_probability = 0.975);
    accumulator_t acc7(quantile_probability = 0.999);


    for (std::size_t i=0; i<100000; ++i)
    {
        double sample = rng();
        acc0(sample, weight = 1.);
        acc1(sample, weight = 1.);
        acc2(sample, weight = 1.);
        acc3(sample, weight = 1.);

        double sample4 = normal4();
        double sample5 = normal5();
        double sample6 = normal6();
        double sample7 = normal7();
        acc4(sample4, weight = std::exp(-mu4 * (sample4 - 0.5 * mu4)));
        acc5(sample5, weight = std::exp(-mu5 * (sample5 - 0.5 * mu5)));
        acc6(sample6, weight = std::exp(-mu6 * (sample6 - 0.5 * mu6)));
        acc7(sample7, weight = std::exp(-mu7 * (sample7 - 0.5 * mu7)));
    }
    // check for uniform distribution with weight = 1
    BOOST_CHECK_CLOSE( weighted_p_square_quantile(acc0), 0.001, 28 );
    BOOST_CHECK_CLOSE( weighted_p_square_quantile(acc1), 0.025, 5 );
    BOOST_CHECK_CLOSE( weighted_p_square_quantile(acc2), 0.975, epsilon );
    BOOST_CHECK_CLOSE( weighted_p_square_quantile(acc3), 0.999, epsilon );

    // check for shifted standard normal distribution ("importance sampling")
    BOOST_CHECK_CLOSE( weighted_p_square_quantile(acc4), -3.090232, epsilon );
    BOOST_CHECK_CLOSE( weighted_p_square_quantile(acc5), -1.959963, epsilon );
    BOOST_CHECK_CLOSE( weighted_p_square_quantile(acc6),  1.959963, epsilon );
    BOOST_CHECK_CLOSE( weighted_p_square_quantile(acc7),  3.090232, epsilon );
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_p_square_quantile test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

