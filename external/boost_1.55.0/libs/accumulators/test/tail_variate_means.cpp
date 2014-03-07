//  (C) Copyright 2006 Eric Niebler, Olivier Gygi.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for tail_variate_means.hpp

#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/numeric/functional/vector.hpp>
#include <boost/accumulators/numeric/functional/complex.hpp>
#include <boost/accumulators/numeric/functional/valarray.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/tail_variate_means.hpp>

using namespace boost;
using namespace unit_test;
using namespace boost::accumulators;

///////////////////////////////////////////////////////////////////////////////
// test_stat
//
void test_stat()
{
    std::size_t c = 5; // cache size

    typedef double variate_type;
    typedef std::vector<variate_type> variate_set_type;

    typedef accumulator_set<double, stats<tag::tail_variate_means<right, variate_set_type, tag::covariate1>(relative)> > accumulator_t1;
    typedef accumulator_set<double, stats<tag::tail_variate_means<right, variate_set_type, tag::covariate1>(absolute)> > accumulator_t2;
    typedef accumulator_set<double, stats<tag::tail_variate_means<left, variate_set_type, tag::covariate1>(relative)> > accumulator_t3;
    typedef accumulator_set<double, stats<tag::tail_variate_means<left, variate_set_type, tag::covariate1>(absolute)> > accumulator_t4;

    accumulator_t1 acc1( right_tail_cache_size = c );
    accumulator_t2 acc2( right_tail_cache_size = c );
    accumulator_t3 acc3( left_tail_cache_size = c );
    accumulator_t4 acc4( left_tail_cache_size = c );

    variate_set_type cov1, cov2, cov3, cov4, cov5;
    double c1[] = { 10., 20., 30., 40. }; // 100
    double c2[] = { 26.,  4., 17.,  3. }; // 50
    double c3[] = { 46., 64., 40., 50. }; // 200
    double c4[] = {  1.,  3., 70.,  6. }; // 80
    double c5[] = {  2.,  2.,  2., 14. }; // 20
    cov1.assign(c1, c1 + sizeof(c1)/sizeof(variate_type));
    cov2.assign(c2, c2 + sizeof(c2)/sizeof(variate_type));
    cov3.assign(c3, c3 + sizeof(c3)/sizeof(variate_type));
    cov4.assign(c4, c4 + sizeof(c4)/sizeof(variate_type));
    cov5.assign(c5, c5 + sizeof(c5)/sizeof(variate_type));

    acc1(100., covariate1 = cov1);
    acc1( 50., covariate1 = cov2);
    acc1(200., covariate1 = cov3);
    acc1( 80., covariate1 = cov4);
    acc1( 20., covariate1 = cov5);

    acc2(100., covariate1 = cov1);
    acc2( 50., covariate1 = cov2);
    acc2(200., covariate1 = cov3);
    acc2( 80., covariate1 = cov4);
    acc2( 20., covariate1 = cov5);

    acc3(100., covariate1 = cov1);
    acc3( 50., covariate1 = cov2);
    acc3(200., covariate1 = cov3);
    acc3( 80., covariate1 = cov4);
    acc3( 20., covariate1 = cov5);

    acc4(100., covariate1 = cov1);
    acc4( 50., covariate1 = cov2);
    acc4(200., covariate1 = cov3);
    acc4( 80., covariate1 = cov4);
    acc4( 20., covariate1 = cov5);

    // check relative risk contributions
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc1, quantile_probability = 0.7).begin()     ), 14./75. ); // (10 + 46) / 300 = 14/75
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc1, quantile_probability = 0.7).begin() + 1),  7./25. ); // (20 + 64) / 300 =  7/25
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc1, quantile_probability = 0.7).begin() + 2),  7./30. ); // (30 + 40) / 300 =  7/30
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc1, quantile_probability = 0.7).begin() + 3),  3./10. ); // (40 + 50) / 300 =  3/10
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc3, quantile_probability = 0.3).begin()    ), 14./35. ); // (26 +  2) /  70 = 14/35
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc3, quantile_probability = 0.3).begin() + 1),  3./35. ); // ( 4 +  2) /  70 =  3/35
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc3, quantile_probability = 0.3).begin() + 2), 19./70. ); // (17 +  2) /  70 = 19/70
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc3, quantile_probability = 0.3).begin() + 3), 17./70. ); // ( 3 + 14) /  70 = 17/70

    // check absolute risk contributions
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc2, quantile_probability = 0.7).begin()    ), 28 ); // (10 + 46) / 2 = 28
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc2, quantile_probability = 0.7).begin() + 1), 42 ); // (20 + 64) / 2 = 42
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc2, quantile_probability = 0.7).begin() + 2), 35 ); // (30 + 40) / 2 = 35
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc2, quantile_probability = 0.7).begin() + 3), 45 ); // (40 + 50) / 2 = 45
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc4, quantile_probability = 0.3).begin()    ), 14 ); // (26 +  2) / 2 = 14
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc4, quantile_probability = 0.3).begin() + 1),  3 ); // ( 4 +  2) / 2 =  3
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc4, quantile_probability = 0.3).begin() + 2),9.5 ); // (17 +  2) / 2 =  9.5
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc4, quantile_probability = 0.3).begin() + 3),8.5 ); // ( 3 + 14) / 2 =  8.5

    // check relative risk contributions
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc1, quantile_probability = 0.9).begin()    ), 23./100. ); // 46/200 = 23/100
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc1, quantile_probability = 0.9).begin() + 1),  8./25.  ); // 64/200 =  8/25
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc1, quantile_probability = 0.9).begin() + 2),  1./5.   ); // 40/200 =  1/5
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc1, quantile_probability = 0.9).begin() + 3),  1./4.   ); // 50/200 =  1/4
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc3, quantile_probability = 0.1).begin()    ),  1./10.  ); //  2/ 20 =  1/10
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc3, quantile_probability = 0.1).begin() + 1),  1./10.  ); //  2/ 20 =  1/10
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc3, quantile_probability = 0.1).begin() + 2),  1./10.  ); //  2/ 20 =  1/10
    BOOST_CHECK_EQUAL( *(relative_tail_variate_means(acc3, quantile_probability = 0.1).begin() + 3),  7./10.  ); // 14/ 20 =  7/10

    // check absolute risk contributions
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc2, quantile_probability = 0.9).begin()    ), 46 ); // 46
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc2, quantile_probability = 0.9).begin() + 1), 64 ); // 64
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc2, quantile_probability = 0.9).begin() + 2), 40 ); // 40
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc2, quantile_probability = 0.9).begin() + 3), 50 ); // 50
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc4, quantile_probability = 0.1).begin()    ),  2 ); //  2
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc4, quantile_probability = 0.1).begin() + 1),  2 ); //  2
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc4, quantile_probability = 0.1).begin() + 2),  2 ); //  2
    BOOST_CHECK_EQUAL( *(tail_variate_means(acc4, quantile_probability = 0.1).begin() + 3), 14 ); // 14
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("tail_variate_means test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

