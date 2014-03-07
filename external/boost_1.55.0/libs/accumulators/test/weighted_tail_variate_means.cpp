//  (C) Copyright 2006 Eric Niebler, Olivier Gygi.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test case for weighted_tail_variate_means.hpp

#define BOOST_NUMERIC_FUNCTIONAL_STD_COMPLEX_SUPPORT
#define BOOST_NUMERIC_FUNCTIONAL_STD_VALARRAY_SUPPORT
#define BOOST_NUMERIC_FUNCTIONAL_STD_VECTOR_SUPPORT

#include <boost/random.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/weighted_tail_variate_means.hpp>

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

    accumulator_set<double, stats<tag::weighted_tail_variate_means<right, variate_set_type, tag::covariate1>(relative)>, double >
        acc1( right_tail_cache_size = c );
    accumulator_set<double, stats<tag::weighted_tail_variate_means<right, variate_set_type, tag::covariate1>(absolute)>, double >
        acc2( right_tail_cache_size = c );
    accumulator_set<double, stats<tag::weighted_tail_variate_means<left, variate_set_type, tag::covariate1>(relative)>, double >
        acc3( left_tail_cache_size = c );
    accumulator_set<double, stats<tag::weighted_tail_variate_means<left, variate_set_type, tag::covariate1>(absolute)>, double >
        acc4( left_tail_cache_size = c );

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

    acc1(100., weight = 0.8, covariate1 = cov1);
    acc1( 50., weight = 0.9, covariate1 = cov2);
    acc1(200., weight = 1.0, covariate1 = cov3);
    acc1( 80., weight = 1.1, covariate1 = cov4);
    acc1( 20., weight = 1.2, covariate1 = cov5);

    acc2(100., weight = 0.8, covariate1 = cov1);
    acc2( 50., weight = 0.9, covariate1 = cov2);
    acc2(200., weight = 1.0, covariate1 = cov3);
    acc2( 80., weight = 1.1, covariate1 = cov4);
    acc2( 20., weight = 1.2, covariate1 = cov5);

    acc3(100., weight = 0.8, covariate1 = cov1);
    acc3( 50., weight = 0.9, covariate1 = cov2);
    acc3(200., weight = 1.0, covariate1 = cov3);
    acc3( 80., weight = 1.1, covariate1 = cov4);
    acc3( 20., weight = 1.2, covariate1 = cov5);

    acc4(100., weight = 0.8, covariate1 = cov1);
    acc4( 50., weight = 0.9, covariate1 = cov2);
    acc4(200., weight = 1.0, covariate1 = cov3);
    acc4( 80., weight = 1.1, covariate1 = cov4);
    acc4( 20., weight = 1.2, covariate1 = cov5);

    // check relative risk contributions
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc1, quantile_probability = 0.7).begin()    ), (0.8*10 + 1.0*46)/(0.8*100 + 1.0*200) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc1, quantile_probability = 0.7).begin() + 1), (0.8*20 + 1.0*64)/(0.8*100 + 1.0*200) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc1, quantile_probability = 0.7).begin() + 2), (0.8*30 + 1.0*40)/(0.8*100 + 1.0*200) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc1, quantile_probability = 0.7).begin() + 3), (0.8*40 + 1.0*50)/(0.8*100 + 1.0*200) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc3, quantile_probability = 0.3).begin()    ), (0.9*26 + 1.2*2)/(0.9*50 + 1.2*20) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc3, quantile_probability = 0.3).begin() + 1), (0.9*4 + 1.2*2)/(0.9*50 + 1.2*20) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc3, quantile_probability = 0.3).begin() + 2), (0.9*17 + 1.2*2)/(0.9*50 + 1.2*20) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc3, quantile_probability = 0.3).begin() + 3), (0.9*3 + 1.2*14)/(0.9*50 + 1.2*20) );

    // check absolute risk contributions
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc2, quantile_probability = 0.7).begin()    ), (0.8*10 + 1.0*46)/1.8 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc2, quantile_probability = 0.7).begin() + 1), (0.8*20 + 1.0*64)/1.8 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc2, quantile_probability = 0.7).begin() + 2), (0.8*30 + 1.0*40)/1.8 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc2, quantile_probability = 0.7).begin() + 3), (0.8*40 + 1.0*50)/1.8 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc4, quantile_probability = 0.3).begin()    ), (0.9*26 + 1.2*2)/2.1 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc4, quantile_probability = 0.3).begin() + 1), (0.9*4 + 1.2*2)/2.1 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc4, quantile_probability = 0.3).begin() + 2), (0.9*17 + 1.2*2)/2.1 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc4, quantile_probability = 0.3).begin() + 3), (0.9*3 + 1.2*14)/2.1 );

    // check relative risk contributions
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc1, quantile_probability = 0.9).begin()    ), 1.0*46/(1.0*200) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc1, quantile_probability = 0.9).begin() + 1), 1.0*64/(1.0*200) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc1, quantile_probability = 0.9).begin() + 2), 1.0*40/(1.0*200) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc1, quantile_probability = 0.9).begin() + 3), 1.0*50/(1.0*200) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc3, quantile_probability = 0.1).begin()    ), 1.2*2/(1.2*20) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc3, quantile_probability = 0.1).begin() + 1), 1.2*2/(1.2*20) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc3, quantile_probability = 0.1).begin() + 2), 1.2*2/(1.2*20) );
    BOOST_CHECK_EQUAL( *(relative_weighted_tail_variate_means(acc3, quantile_probability = 0.1).begin() + 3), 1.2*14/(1.2*20) );

    // check absolute risk contributions
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc2, quantile_probability = 0.9).begin()    ), 1.0*46/1.0 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc2, quantile_probability = 0.9).begin() + 1), 1.0*64/1.0 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc2, quantile_probability = 0.9).begin() + 2), 1.0*40/1.0 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc2, quantile_probability = 0.9).begin() + 3), 1.0*50/1.0 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc4, quantile_probability = 0.1).begin()    ), 1.2*2/1.2 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc4, quantile_probability = 0.1).begin() + 1), 1.2*2/1.2 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc4, quantile_probability = 0.1).begin() + 2), 1.2*2/1.2 );
    BOOST_CHECK_EQUAL( *(weighted_tail_variate_means(acc4, quantile_probability = 0.1).begin() + 3), 1.2*14/1.2 );
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("weighted_tail_variate_means test");

    test->add(BOOST_TEST_CASE(&test_stat));

    return test;
}

