/* test_discrete_distribution.cpp
 *
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_discrete_distribution.cpp 85813 2013-09-21 20:17:00Z jewillco $
 *
 */

#include <boost/random/discrete_distribution.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/assign/list_of.hpp>
#include <sstream>
#include <vector>
#include "concepts.hpp"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using boost::random::test::RandomNumberDistribution;
using boost::random::discrete_distribution;
BOOST_CONCEPT_ASSERT((RandomNumberDistribution< discrete_distribution<> >));

struct gen {
    double operator()(double arg) {
        if(arg < 100) return 100;
        else if(arg < 103) return 1;
        else if(arg < 107) return 2;
        else if(arg < 111) return 1;
        else if(arg < 114) return 4;
        else return 100;
    }
};

#define CHECK_PROBABILITIES(actual, expected)       \
    do {                                            \
        std::vector<double> _actual = (actual);     \
        std::vector<double> _expected = (expected); \
        BOOST_CHECK_EQUAL_COLLECTIONS(              \
            _actual.begin(), _actual.end(),         \
            _expected.begin(), _expected.end());    \
    } while(false)

using boost::assign::list_of;

BOOST_AUTO_TEST_CASE(test_constructors) {
    boost::random::discrete_distribution<> dist;
    CHECK_PROBABILITIES(dist.probabilities(), list_of(1.0));

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    boost::random::discrete_distribution<> dist_il = { 1, 2, 1, 4 };
    CHECK_PROBABILITIES(dist_il.probabilities(), list_of(.125)(.25)(.125)(.5));
#endif
    std::vector<double> probs = boost::assign::list_of(1.0)(2.0)(1.0)(4.0);

    boost::random::discrete_distribution<> dist_r(probs);
    CHECK_PROBABILITIES(dist_r.probabilities(), list_of(.125)(.25)(.125)(.5));
    
    boost::random::discrete_distribution<> dist_it(probs.begin(), probs.end());
    CHECK_PROBABILITIES(dist_it.probabilities(), list_of(.125)(.25)(.125)(.5));
    
    boost::random::discrete_distribution<> dist_fun(4, 99, 115, gen());
    CHECK_PROBABILITIES(dist_fun.probabilities(), list_of(.125)(.25)(.125)(.5));

    boost::random::discrete_distribution<> copy(dist);
    BOOST_CHECK_EQUAL(dist, copy);
    boost::random::discrete_distribution<> copy_r(dist_r);
    BOOST_CHECK_EQUAL(dist_r, copy_r);

    boost::random::discrete_distribution<> notpow2(3, 99, 111, gen());
    BOOST_REQUIRE_EQUAL(notpow2.probabilities().size(), 3u);
    BOOST_CHECK_CLOSE_FRACTION(notpow2.probabilities()[0], 0.25, 0.00000000001);
    BOOST_CHECK_CLOSE_FRACTION(notpow2.probabilities()[1], 0.50, 0.00000000001);
    BOOST_CHECK_CLOSE_FRACTION(notpow2.probabilities()[2], 0.25, 0.00000000001);
    boost::random::discrete_distribution<> copy_notpow2(notpow2);
    BOOST_CHECK_EQUAL(notpow2, copy_notpow2);
}

BOOST_AUTO_TEST_CASE(test_param) {
    std::vector<double> probs = boost::assign::list_of(1.0)(2.0)(1.0)(4.0);
    boost::random::discrete_distribution<> dist(probs);
    boost::random::discrete_distribution<>::param_type param = dist.param();
    CHECK_PROBABILITIES(param.probabilities(), list_of(.125)(.25)(.125)(.5));
    boost::random::discrete_distribution<> copy1(param);
    BOOST_CHECK_EQUAL(dist, copy1);
    boost::random::discrete_distribution<> copy2;
    copy2.param(param);
    BOOST_CHECK_EQUAL(dist, copy2);

    boost::random::discrete_distribution<>::param_type param_copy = param;
    BOOST_CHECK_EQUAL(param, param_copy);
    BOOST_CHECK(param == param_copy);
    BOOST_CHECK(!(param != param_copy));
    boost::random::discrete_distribution<>::param_type param_default;
    CHECK_PROBABILITIES(param_default.probabilities(), list_of(1.0));
    BOOST_CHECK(param != param_default);
    BOOST_CHECK(!(param == param_default));
    
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    boost::random::discrete_distribution<>::param_type
        parm_il = { 1, 2, 1, 4 };
    CHECK_PROBABILITIES(parm_il.probabilities(), list_of(.125)(.25)(.125)(.5));
#endif

    boost::random::discrete_distribution<>::param_type parm_r(probs);
    CHECK_PROBABILITIES(parm_r.probabilities(), list_of(.125)(.25)(.125)(.5));
    
    boost::random::discrete_distribution<>::param_type
        parm_it(probs.begin(), probs.end());
    CHECK_PROBABILITIES(parm_it.probabilities(), list_of(.125)(.25)(.125)(.5));
    
    boost::random::discrete_distribution<>::param_type
        parm_fun(4, 99, 115, gen());
    CHECK_PROBABILITIES(parm_fun.probabilities(), list_of(.125)(.25)(.125)(.5));
}

BOOST_AUTO_TEST_CASE(test_min_max) {
    std::vector<double> probs = boost::assign::list_of(1.0)(2.0)(1.0);
    boost::random::discrete_distribution<> dist;
    BOOST_CHECK_EQUAL((dist.min)(), 0);
    BOOST_CHECK_EQUAL((dist.max)(), 0);
    boost::random::discrete_distribution<> dist_r(probs);
    BOOST_CHECK_EQUAL((dist_r.min)(), 0);
    BOOST_CHECK_EQUAL((dist_r.max)(), 2);
}

BOOST_AUTO_TEST_CASE(test_comparison) {
    std::vector<double> probs = boost::assign::list_of(1.0)(2.0)(1.0)(4.0);
    boost::random::discrete_distribution<> dist;
    boost::random::discrete_distribution<> dist_copy(dist);
    boost::random::discrete_distribution<> dist_r(probs);
    boost::random::discrete_distribution<> dist_r_copy(dist_r);
    BOOST_CHECK(dist == dist_copy);
    BOOST_CHECK(!(dist != dist_copy));
    BOOST_CHECK(dist_r == dist_r_copy);
    BOOST_CHECK(!(dist_r != dist_r_copy));
    BOOST_CHECK(dist != dist_r);
    BOOST_CHECK(!(dist == dist_r));
}

BOOST_AUTO_TEST_CASE(test_streaming) {
    std::vector<double> probs = boost::assign::list_of(1.0)(2.0)(1.0)(4.0);
    boost::random::discrete_distribution<> dist(probs);
    std::stringstream stream;
    stream << dist;
    boost::random::discrete_distribution<> restored_dist;
    stream >> restored_dist;
    BOOST_CHECK_EQUAL(dist, restored_dist);
}

BOOST_AUTO_TEST_CASE(test_generation) {
    std::vector<double> probs = boost::assign::list_of(0.0)(1.0);
    boost::minstd_rand0 gen;
    boost::random::discrete_distribution<> dist;
    boost::random::discrete_distribution<> dist_r(probs);
    for(int i = 0; i < 10; ++i) {
        int value = dist(gen);
        BOOST_CHECK_EQUAL(value, 0);
        int value_r = dist_r(gen);
        BOOST_CHECK_EQUAL(value_r, 1);
        int value_param = dist_r(gen, dist.param());
        BOOST_CHECK_EQUAL(value_param, 0);
        int value_r_param = dist(gen, dist_r.param());
        BOOST_CHECK_EQUAL(value_r_param, 1);
    }
}
