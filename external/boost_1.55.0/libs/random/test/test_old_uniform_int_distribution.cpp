/* test_old_uniform_int_distribution.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_old_uniform_int_distribution.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/uniform_int.hpp>
#include <limits>

#define BOOST_RANDOM_DISTRIBUTION boost::uniform_int<>
#define BOOST_RANDOM_ARG1 a
#define BOOST_RANDOM_ARG2 b
#define BOOST_RANDOM_ARG1_DEFAULT 0
#define BOOST_RANDOM_ARG2_DEFAULT 9
#define BOOST_RANDOM_ARG1_VALUE 5
#define BOOST_RANDOM_ARG2_VALUE 250

#define BOOST_RANDOM_DIST0_MIN 0
#define BOOST_RANDOM_DIST0_MAX 9
#define BOOST_RANDOM_DIST1_MIN 5
#define BOOST_RANDOM_DIST1_MAX 9
#define BOOST_RANDOM_DIST2_MIN 5
#define BOOST_RANDOM_DIST2_MAX 250

#define BOOST_RANDOM_TEST1_PARAMS (0, 9)
#define BOOST_RANDOM_TEST1_MIN 0
#define BOOST_RANDOM_TEST1_MAX 9

#define BOOST_RANDOM_TEST2_PARAMS (10, 19)
#define BOOST_RANDOM_TEST2_MIN 10
#define BOOST_RANDOM_TEST2_MAX 19

#include "test_distribution.ipp"

#define BOOST_RANDOM_UNIFORM_INT boost::uniform_int

#include "test_uniform_int.ipp"

#include <algorithm>
#include <boost/random/random_number_generator.hpp>

// Test that uniform_int<> can be used with std::random_shuffle
// Author: Jos Hickson
BOOST_AUTO_TEST_CASE(test_random_shuffle)
{
    typedef boost::uniform_int<> distribution_type;
    typedef boost::variate_generator<boost::mt19937 &, distribution_type> generator_type;

    boost::mt19937 engine1(1234);
    boost::mt19937 engine2(1234);

    boost::random::random_number_generator<boost::mt19937> referenceRand(engine1);

    distribution_type dist(0,10);
    generator_type testRand(engine2, dist);

    std::vector<int> referenceVec;

    for (int i = 0; i < 200; ++i) {
        referenceVec.push_back(i);
    }

    std::vector<int> testVec(referenceVec);

    std::random_shuffle(referenceVec.begin(), referenceVec.end(), referenceRand);
    std::random_shuffle(testVec.begin(), testVec.end(), testRand);

    BOOST_CHECK_EQUAL_COLLECTIONS(
        testVec.begin(), testVec.end(),
        referenceVec.begin(), referenceVec.end());
}
