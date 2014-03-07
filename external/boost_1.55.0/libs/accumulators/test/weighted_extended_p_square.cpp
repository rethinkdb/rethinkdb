//  (C) Copyright Eric Niebler 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for weighted_extended_p_square.hpp

#include <iostream>
#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/numeric/functional/complex.hpp>
#include <boost/accumulators/numeric/functional/valarray.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_extended_p_square.hpp>

using namespace boost;
using namespace unit_test;
using namespace boost::accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    typedef accumulator_set<double, stats<tag::weighted_extended_p_square>, double> accumulator_t;

    // problem with small results: epsilon is relative (in percent), not absolute

    // tolerance in %
    double epsilon = 1;

    // some random number generators
    double mu1 = -1.0;
    double mu2 =  1.0;
    boost::lagged_fibonacci607 rng;
    boost::normal_distribution<> mean_sigma1(mu1, 1);
    boost::normal_distribution<> mean_sigma2(mu2, 1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal1(rng, mean_sigma1);
    boost::variate_generator<boost::lagged_fibonacci607&, boost::normal_distribution<> > normal2(rng, mean_sigma2);

    std::vector<double> probs_uniform, probs_normal1, probs_normal2, probs_normal_exact1, probs_normal_exact2;

    double p1[] = {/*0.001,*/ 0.01, 0.1, 0.5, 0.9, 0.99, 0.999};
    probs_uniform.assign(p1, p1 + sizeof(p1) / sizeof(double));

    double p2[] = {0.001, 0.025};
    double p3[] = {0.975, 0.999};
    probs_normal1.assign(p2, p2 + sizeof(p2) / sizeof(double));
    probs_normal2.assign(p3, p3 + sizeof(p3) / sizeof(double));

    double p4[] = {-3.090232, -1.959963};
    double p5[] = {1.959963, 3.090232};
    probs_normal_exact1.assign(p4, p4 + sizeof(p4) / sizeof(double));
    probs_normal_exact2.assign(p5, p5 + sizeof(p5) / sizeof(double));

    accumulator_t acc_uniform(extended_p_square_probabilities = probs_uniform);
    accumulator_t acc_normal1(extended_p_square_probabilities = probs_normal1);
    accumulator_t acc_normal2(extended_p_square_probabilities = probs_normal2);

    for (std::size_t i = 0; i < 100000; ++i)
    {
        acc_uniform(rng(), weight = 1.);

        double sample1 = normal1();
        double sample2 = normal2();
        acc_normal1(sample1, weight = std::exp(-mu1 * (sample1 - 0.5 * mu1)));
        acc_normal2(sample2, weight = std::exp(-mu2 * (sample2 - 0.5 * mu2)));
    }

    // check for uniform distribution
    BOOST_CHECK_CLOSE(weighted_extended_p_square(acc_uniform)[0], probs_uniform[0], 6*epsilon);
    BOOST_CHECK_CLOSE(weighted_extended_p_square(acc_uniform)[1], probs_uniform[1], 3*epsilon);
    BOOST_CHECK_CLOSE(weighted_extended_p_square(acc_uniform)[2], probs_uniform[2], epsilon);
    BOOST_CHECK_CLOSE(weighted_extended_p_square(acc_uniform)[3], probs_uniform[3], epsilon);
    BOOST_CHECK_CLOSE(weighted_extended_p_square(acc_uniform)[4], probs_uniform[4], epsilon);
    BOOST_CHECK_CLOSE(weighted_extended_p_square(acc_uniform)[5], probs_uniform[5], epsilon);

    // check for standard normal distribution
    for (std::size_t i = 0; i < probs_normal1.size(); ++i)
    {
        BOOST_CHECK_CLOSE(weighted_extended_p_square(acc_normal1)[i], probs_normal_exact1[i], epsilon);
        BOOST_CHECK_CLOSE(weighted_extended_p_square(acc_normal2)[i], probs_normal_exact2[i], epsilon);
    }
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_extended_p_square test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

