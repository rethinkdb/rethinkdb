/* test_bernoulli_distribution.cpp
 *
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_bernoulli_distribution.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/bernoulli_distribution.hpp>

#define BOOST_RANDOM_DISTRIBUTION boost::random::bernoulli_distribution<>
#define BOOST_RANDOM_ARG1 p
#define BOOST_RANDOM_ARG1_DEFAULT 0.5
#define BOOST_RANDOM_ARG1_VALUE 0.25

#define BOOST_RANDOM_DIST0_MIN false
#define BOOST_RANDOM_DIST0_MAX true
#define BOOST_RANDOM_DIST1_MIN false
#define BOOST_RANDOM_DIST1_MAX true

#define BOOST_RANDOM_TEST1_PARAMS (0.0)
#define BOOST_RANDOM_TEST1_MIN false
#define BOOST_RANDOM_TEST1_MAX false

#define BOOST_RANDOM_TEST2_PARAMS (1.0)
#define BOOST_RANDOM_TEST2_MIN true
#define BOOST_RANDOM_TEST2_MAX true

#include "test_distribution.ipp"
