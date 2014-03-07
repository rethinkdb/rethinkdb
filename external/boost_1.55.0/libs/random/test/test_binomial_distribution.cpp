/* test_binomial_distribution.cpp
 *
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_binomial_distribution.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/binomial_distribution.hpp>

#define BOOST_RANDOM_DISTRIBUTION boost::random::binomial_distribution<>
#define BOOST_RANDOM_ARG1 t
#define BOOST_RANDOM_ARG2 p
#define BOOST_RANDOM_ARG1_DEFAULT 1
#define BOOST_RANDOM_ARG2_DEFAULT 0.5
#define BOOST_RANDOM_ARG1_VALUE 10
#define BOOST_RANDOM_ARG2_VALUE 0.25

#define BOOST_RANDOM_DIST0_MIN 0
#define BOOST_RANDOM_DIST0_MAX 1
#define BOOST_RANDOM_DIST1_MIN 0
#define BOOST_RANDOM_DIST1_MAX 10
#define BOOST_RANDOM_DIST2_MIN 0
#define BOOST_RANDOM_DIST2_MAX 10

#define BOOST_RANDOM_TEST1_PARAMS
#define BOOST_RANDOM_TEST1_MIN 0
#define BOOST_RANDOM_TEST1_MAX 1

#define BOOST_RANDOM_TEST2_PARAMS (10, 0.25)
#define BOOST_RANDOM_TEST2_MIN 0
#define BOOST_RANDOM_TEST2_MAX 10

#include "test_distribution.ipp"
