/* test_uniform_int_distribution.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_uniform_int_distribution.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/uniform_int_distribution.hpp>
#include <limits>

#define BOOST_RANDOM_DISTRIBUTION boost::random::uniform_int_distribution<>
#define BOOST_RANDOM_ARG1 a
#define BOOST_RANDOM_ARG2 b
#define BOOST_RANDOM_ARG1_DEFAULT 0
#define BOOST_RANDOM_ARG2_DEFAULT 0x7fffffff
#define BOOST_RANDOM_ARG1_VALUE 100
#define BOOST_RANDOM_ARG2_VALUE 250

#define BOOST_RANDOM_DIST0_MIN 0
#define BOOST_RANDOM_DIST0_MAX 0x7fffffff
#define BOOST_RANDOM_DIST1_MIN 100
#define BOOST_RANDOM_DIST1_MAX 0x7fffffff
#define BOOST_RANDOM_DIST2_MIN 100
#define BOOST_RANDOM_DIST2_MAX 250

#define BOOST_RANDOM_TEST1_PARAMS (0, 9)
#define BOOST_RANDOM_TEST1_MIN 0
#define BOOST_RANDOM_TEST1_MAX 9

#define BOOST_RANDOM_TEST2_PARAMS (10, 19)
#define BOOST_RANDOM_TEST2_MIN 10
#define BOOST_RANDOM_TEST2_MAX 19

#include "test_distribution.ipp"

#define BOOST_RANDOM_UNIFORM_INT boost::random::uniform_int_distribution

#include "test_uniform_int.ipp"
