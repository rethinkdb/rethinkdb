//  (C) Copyright 2006 Eric Niebler, Olivier Gygi
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/random.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    // two random number generators
    double mu = 1.;
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma(mu,1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal(rng, mean_sigma);

    accumulator_set<double, stats<tag::median(with_p_square_quantile) > > acc;
    accumulator_set<double, stats<tag::median(with_density) > >
        acc_dens( density_cache_size = 10000, density_num_bins = 1000 );
    accumulator_set<double, stats<tag::median(with_p_square_cumulative_distribution) > >
        acc_cdist( p_square_cumulative_distribution_num_cells = 100 );

    for (std::size_t i=0; i<100000; ++i)
    {
        double sample = normal();
        acc(sample);
        acc_dens(sample);
        acc_cdist(sample);
    }

    BOOST_CHECK_CLOSE(1., median(acc), 1.);
    BOOST_CHECK_CLOSE(1., median(acc_dens), 1.);
    BOOST_CHECK_CLOSE(1., median(acc_cdist), 3.);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("median test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
