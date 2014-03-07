/* test_cauchy.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_cauchy.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/cauchy_distribution.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/math/distributions/cauchy.hpp>

#define BOOST_RANDOM_DISTRIBUTION boost::random::cauchy_distribution<>
#define BOOST_RANDOM_DISTRIBUTION_NAME cauchy
#define BOOST_MATH_DISTRIBUTION boost::math::cauchy
#define BOOST_RANDOM_ARG1_TYPE double
#define BOOST_RANDOM_ARG1_NAME median
#define BOOST_RANDOM_ARG1_DEFAULT 1000.0
#define BOOST_RANDOM_ARG1_DISTRIBUTION(n) boost::uniform_real<>(-n, n)
#define BOOST_RANDOM_ARG2_TYPE double
#define BOOST_RANDOM_ARG2_NAME sigma
#define BOOST_RANDOM_ARG2_DEFAULT 1000.0
#define BOOST_RANDOM_ARG2_DISTRIBUTION(n) boost::uniform_real<>(0.001, n)

#include "test_real_distribution.ipp"
