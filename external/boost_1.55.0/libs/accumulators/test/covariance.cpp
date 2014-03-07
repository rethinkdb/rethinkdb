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
#include <boost/accumulators/statistics/covariance.hpp>

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

    accumulator_set<double, stats<tag::covariance<double, tag::covariate1> > > acc;
    accumulator_set<std::vector<double>, stats<tag::covariance<double, tag::covariate1> > > acc2(sample = dummy);
    accumulator_set<double, stats<tag::covariance<std::vector<double>, tag::covariate1> > > acc3(covariate1 = dummy);
    accumulator_set<std::vector<double>, stats<tag::covariance<std::vector<double>, tag::covariate1> > > acc4(sample = dummy, covariate1 = dummy);

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
        acc(1., covariate1 = 2.);
        acc(1., covariate1 = 4.);
        acc(2., covariate1 = 3.);
        acc(6., covariate1 = 1.);
    }

    // vector - double
    {
        acc2(a, covariate1 = 1.);
        acc2(b, covariate1 = 1.);
        acc2(c, covariate1 = 2.);
        acc2(d, covariate1 = 6.);
    }

    // double - vector
    {
        acc3(1., covariate1 = a);
        acc3(1., covariate1 = b);
        acc3(2., covariate1 = c);
        acc3(6., covariate1 = d);
    }

    // vector - vector
    {
        acc4(a, covariate1 = b);
        acc4(b, covariate1 = c);
        acc4(a, covariate1 = c);
        acc4(d, covariate1 = b);
    }

    double epsilon = 1e-6;

    BOOST_CHECK_CLOSE((covariance(acc)), -1.75, epsilon);
    BOOST_CHECK_CLOSE((covariance(acc2))[0], 1.75, epsilon);
    BOOST_CHECK_CLOSE((covariance(acc2))[1], -1.125, epsilon);
    BOOST_CHECK_CLOSE((covariance(acc3))[0], 1.75, epsilon);
    BOOST_CHECK_CLOSE((covariance(acc3))[1], -1.125, epsilon);
    BOOST_CHECK_CLOSE((covariance(acc4))(0,0), 0.125, epsilon);
    BOOST_CHECK_CLOSE((covariance(acc4))(0,1), -0.25, epsilon);
    BOOST_CHECK_CLOSE((covariance(acc4))(1,0), -0.125, epsilon);
    BOOST_CHECK_CLOSE((covariance(acc4))(1,1), 0.25, epsilon);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("covariance test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}
