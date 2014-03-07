/* test_generate_canonical.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_generate_canonical.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/generate_canonical.hpp>

#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/lagged_fibonacci.hpp>
#include <boost/cstdint.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

typedef boost::mpl::vector<
    boost::random::minstd_rand,
    boost::random::mt19937,
    boost::random::lagged_fibonacci607
> engines;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_float, Engine, engines)
{
    Engine eng;
    Engine expected;
    for(int i = 0; i < 1000; ++i) {
        float val = boost::random::generate_canonical<float, 64>(eng);
        BOOST_CHECK_GE(val, 0);
        BOOST_CHECK_LT(val, 1);
    }
    expected.discard(1000);
    BOOST_CHECK_EQUAL(eng, expected);
    for(int i = 0; i < 1000; ++i) {
        float val = boost::random::generate_canonical<float, 12>(eng);
        BOOST_CHECK_GE(val, 0);
        BOOST_CHECK_LT(val, 1);
    }
    expected.discard(1000);
    BOOST_CHECK_EQUAL(eng, expected);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_double, Engine, engines)
{
    Engine eng;
    Engine expected;
    for(int i = 0; i < 1000; ++i) {
        double val = boost::random::generate_canonical<double, 64>(eng);
        BOOST_CHECK_GE(val, 0);
        BOOST_CHECK_LT(val, 1);
    }
    expected.discard(2000);
    BOOST_CHECK_EQUAL(eng, expected);
    for(int i = 0; i < 1000; ++i) {
        double val = boost::random::generate_canonical<double, 12>(eng);
        BOOST_CHECK_GE(val, 0);
        BOOST_CHECK_LT(val, 1);
    }
    expected.discard(1000);
    BOOST_CHECK_EQUAL(eng, expected);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_long_double, Engine, engines)
{
    Engine eng;
    Engine expected;
    for(int i = 0; i < 1000; ++i) {
        long double val = boost::random::generate_canonical<long double, 60>(eng);
        BOOST_CHECK_GE(val, 0);
        BOOST_CHECK_LT(val, 1);
    }
    expected.discard(2000);
    BOOST_CHECK_EQUAL(eng, expected);
    for(int i = 0; i < 1000; ++i) {
        long double val = boost::random::generate_canonical<long double, 12>(eng);
        BOOST_CHECK_GE(val, 0);
        BOOST_CHECK_LT(val, 1);
    }
    expected.discard(1000);
    BOOST_CHECK_EQUAL(eng, expected);
}

struct max_engine
{
    typedef boost::uint32_t result_type;
    static result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
    static result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return ~boost::uint32_t(0); }
    result_type operator()() { return (max)(); }
};

BOOST_AUTO_TEST_CASE(test_max)
{
    max_engine eng;
    BOOST_CHECK_LT((boost::random::generate_canonical<float, 64>(eng)), 1);
    BOOST_CHECK_LT((boost::random::generate_canonical<double, 64>(eng)), 1);
    BOOST_CHECK_LT((boost::random::generate_canonical<long double, 64>(eng)), 1);
}
