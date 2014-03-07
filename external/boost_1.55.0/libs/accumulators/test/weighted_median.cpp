//  (C) Copyright 2006 Eric Niebler, Olivier Gygi
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/random.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_median.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    // Median estimation of normal distribution N(1,1) using samples from a narrow normal distribution N(1,0.01)
    // The weights equal to the likelihood ratio of the corresponding samples

    // two random number generators
    double mu = 1.;
    double sigma_narrow = 0.01;
    double sigma = 1.;
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma_narrow(mu,sigma_narrow);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal_narrow(rng, mean_sigma_narrow);

    accumulator_set<double, stats<tag::weighted_median(with_p_square_quantile) >, double > acc;
    accumulator_set<double, stats<tag::weighted_median(with_density) >, double >
        acc_dens( density_cache_size = 10000, density_num_bins = 1000 );
    accumulator_set<double, stats<tag::weighted_median(with_p_square_cumulative_distribution) >, double >
        acc_cdist( p_square_cumulative_distribution_num_cells = 100 );


    for (std::size_t i=0; i<100000; ++i)
    {
        double sample = normal_narrow();
        acc(sample, weight = std::exp(0.5 * (sample - mu) * (sample - mu) * ( 1./sigma_narrow/sigma_narrow - 1./sigma/sigma )));
        acc_dens(sample, weight = std::exp(0.5 * (sample - mu) * (sample - mu) * ( 1./sigma_narrow/sigma_narrow - 1./sigma/sigma )));
        acc_cdist(sample, weight = std::exp(0.5 * (sample - mu) * (sample - mu) * ( 1./sigma_narrow/sigma_narrow - 1./sigma/sigma )));
    }

    BOOST_CHECK_CLOSE(1., weighted_median(acc), 2);
    BOOST_CHECK_CLOSE(1., weighted_median(acc_dens), 3);
    BOOST_CHECK_CLOSE(1., weighted_median(acc_cdist), 3);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_median test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
