/* test_poisson_distribution.cpp
 *
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_poisson_distribution.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/poisson_distribution.hpp>

#include <limits>

#define BOOST_RANDOM_DISTRIBUTION boost::random::poisson_distribution<>
#define BOOST_RANDOM_ARG1 mean
#define BOOST_RANDOM_ARG1_DEFAULT 1.0
#define BOOST_RANDOM_ARG1_VALUE 7.5

#define BOOST_RANDOM_DIST0_MIN 0
#define BOOST_RANDOM_DIST0_MAX (std::numeric_limits<int>::max)()
#define BOOST_RANDOM_DIST1_MIN 0
#define BOOST_RANDOM_DIST1_MAX (std::numeric_limits<int>::max)()

#define BOOST_RANDOM_TEST1_PARAMS
#define BOOST_RANDOM_TEST1_MIN 0.0
#define BOOST_RANDOM_TEST1_MAX 10.0

#define BOOST_RANDOM_TEST2_PARAMS (1000.0)
#define BOOST_RANDOM_TEST2_MIN 10.0

#include "test_distribution.ipp"
