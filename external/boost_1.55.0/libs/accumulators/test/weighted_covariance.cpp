//  (C) Copyright 2005 Daniel Egloff, Eric Niebler
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_NUMERIC_FUNCTIONAL_STD_VECTOR_SUPPORT

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/weighted_covariance.hpp>

using namespace boost;
using namespace unit_test;
using namespace accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    std::vector<double> dummy;
    dummy.push_back(0);
    dummy.push_back(0);

    accumulator_set<double, stats<tag::weighted_covariance<double, tag::covariate1> >, double > acc;
    accumulator_set<std::vector<double>, stats<tag::weighted_covariance<double, tag::covariate1> >, double > acc2(sample = dummy);
    accumulator_set<double, stats<tag::weighted_covariance<std::vector<double>, tag::covariate1> >, double > acc3(covariate1 = dummy);
    accumulator_set<std::vector<double>, stats<tag::weighted_covariance<std::vector<double>, tag::covariate1> >, double > acc4(sample = dummy, covariate1 = dummy);

    std::vector<double> a;
    a.push_back(1.);
    a.push_back(2.);
    std::vector<double> b;
    b.push_back(3.);
    b.push_back(4.);
    std::vector<double> c;
    c.push_back(2.);
    c.push_back(5.);
    std::vector<double> d;
    d.push_back(4.);
    d.push_back(2.);

    // double - double
    {
        acc(1., weight = 1.1, covariate1 = 2.);
        acc(1., weight = 2.2, covariate1 = 4.);
        acc(2., weight = 3.3, covariate1 = 3.);
        acc(6., weight = 4.4, covariate1 = 1.);
    }

    // vector - double
    {
        acc2(a, weight = 1.1, covariate1 = 1.);
        acc2(b, weight = 2.2, covariate1 = 1.);
        acc2(c, weight = 3.3, covariate1 = 2.);
        acc2(d, weight = 4.4, covariate1 = 6.);
    }

    // double - vector
    {
        acc3(1., weight = 1.1, covariate1 = a);
        acc3(1., weight = 2.2, covariate1 = b);
        acc3(2., weight = 3.3, covariate1 = c);
        acc3(6., weight = 4.4, covariate1 = d);
    }

    // vector - vector
    {
        acc4(a, weight = 1.1, covariate1 = b);
        acc4(b, weight = 2.2, covariate1 = c);
        acc4(a, weight = 3.3, covariate1 = c);
        acc4(d, weight = 4.4, covariate1 = b);
    }

    double epsilon = 1e-6;

    BOOST_CHECK_CLOSE((weighted_covariance(acc)), -2.39, epsilon);
    BOOST_CHECK_CLOSE((weighted_covariance(acc2))[0], 1.93, epsilon);
    BOOST_CHECK_CLOSE((weighted_covariance(acc2))[1], -2.09, epsilon);
    BOOST_CHECK_CLOSE((weighted_covariance(acc3))[0], 1.93, epsilon);
    BOOST_CHECK_CLOSE((weighted_covariance(acc3))[1], -2.09, epsilon);
    BOOST_CHECK_CLOSE((weighted_covariance(acc4))(0,0), 0.4, epsilon);
    BOOST_CHECK_CLOSE((weighted_covariance(acc4))(0,1), -0.2, epsilon);
    BOOST_CHECK_CLOSE((weighted_covariance(acc4))(1,0), -0.4, epsilon);
    BOOST_CHECK_CLOSE((weighted_covariance(acc4))(1,1), 0.2, epsilon);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_covariance test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
