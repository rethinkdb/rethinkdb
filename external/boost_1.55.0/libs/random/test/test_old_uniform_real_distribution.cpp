/* test_old_uniform_real_distribution.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_old_uniform_real_distribution.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/uniform_real.hpp>
#include <limits>

#define BOOST_RANDOM_DISTRIBUTION boost::uniform_real<>
#define BOOST_RANDOM_ARG1 a
#define BOOST_RANDOM_ARG2 b
#define BOOST_RANDOM_ARG1_DEFAULT 0.0
#define BOOST_RANDOM_ARG2_DEFAULT 1.0
#define BOOST_RANDOM_ARG1_VALUE -0.5
#define BOOST_RANDOM_ARG2_VALUE 1.5

#define BOOST_RANDOM_DIST0_MIN 0.0
#define BOOST_RANDOM_DIST0_MAX 1.0
#define BOOST_RANDOM_DIST1_MIN -0.5
#define BOOST_RANDOM_DIST1_MAX 1.0
#define BOOST_RANDOM_DIST2_MIN -0.5
#define BOOST_RANDOM_DIST2_MAX 1.5

#define BOOST_RANDOM_TEST1_PARAMS (-1.0, 0.0)
#define BOOST_RANDOM_TEST1_MIN -1.0
#define BOOST_RANDOM_TEST1_MAX 0.0

#define BOOST_RANDOM_TEST2_PARAMS
#define BOOST_RANDOM_TEST2_MIN 0.0
#define BOOST_RANDOM_TEST2_MAX 1.0

#include "test_distribution.ipp"
