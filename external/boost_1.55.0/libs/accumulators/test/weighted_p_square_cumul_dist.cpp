//  (C) Copyright Eric Niebler, Olivier Gygi 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for weighted_p_square_cumul_dist.hpp

#include <cmath>
#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/numeric/functional/complex.hpp>
#include <boost/accumulators/numeric/functional/valarray.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_p_square_cumul_dist.hpp>

using namespace boost;
using namespace unit_test;
using namespace boost::accumulators;

///////////////////////////////////////////////////////////////////////////////
// erf() not known by VC++ compiler!
// my_erf() computes error function by numerically integrating with trapezoidal rule
//
double my_erf(double const& x, int const& n = 1000)
{
    double sum = 0.;
    double delta = x/n;
    for (int i = 1; i < n; ++i)
        sum += std::exp(-i*i*delta*delta) * delta;
    sum += 0.5 * delta * (1. + std::exp(-x*x));
    return sum * 2. / std::sqrt(3.141592653);
}

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    // tolerance in %
    double epsilon = 4;

    typedef accumulator_set<double, stats<tag::weighted_p_square_cumulative_distribution>, double > accumulator_t;

    accumulator_t acc_upper(p_square_cumulative_distribution_num_cells = 100);
    accumulator_t acc_lower(p_square_cumulative_distribution_num_cells = 100);

    // two random number generators
    double mu_upper = 1.0;
    double mu_lower = -1.0;
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma_upper(mu_upper,1);
    boost::normal_distribution<> mean_sigma_lower(mu_lower,1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal_upper(rng, mean_sigma_upper);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal_lower(rng, mean_sigma_lower);

    for (std::size_t i=0; i<100000; ++i)
    {
        double sample = normal_upper();
        acc_upper(sample, weight = std::exp(-mu_upper * (sample - 0.5 * mu_upper)));
    }

    for (std::size_t i=0; i<100000; ++i)
    {
        double sample = normal_lower();
        acc_lower(sample, weight = std::exp(-mu_lower * (sample - 0.5 * mu_lower)));
    }

    typedef iterator_range<std::vector<std::pair<double, double> >::iterator > histogram_type;
    histogram_type histogram_upper = weighted_p_square_cumulative_distribution(acc_upper);
    histogram_type histogram_lower = weighted_p_square_cumulative_distribution(acc_lower);

    // Note that applying importance sampling results in a region of the distribution
    // to be estimated more accurately and another region to be estimated less accurately
    // than without importance sampling, i.e., with unweighted samples

    for (std::size_t i = 0; i < histogram_upper.size(); ++i)
    {
        // problem with small results: epsilon is relative (in percent), not absolute!

        // check upper region of distribution
        if ( histogram_upper[i].second > 0.1 )
            BOOST_CHECK_CLOSE( 0.5 * (1.0 + my_erf( histogram_upper[i].first / std::sqrt(2.0) )), histogram_upper[i].second, epsilon );
        // check lower region of distribution
        if ( histogram_lower[i].second < -0.1 )
            BOOST_CHECK_CLOSE( 0.5 * (1.0 + my_erf( histogram_lower[i].first / std::sqrt(2.0) )), histogram_lower[i].second, epsilon );
    }
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_p_square_cumulative_distribution test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

