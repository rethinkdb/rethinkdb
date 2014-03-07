/* test_triangle.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_triangle.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/triangle_distribution.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/math/distributions/triangular.hpp>

#define BOOST_RANDOM_DISTRIBUTION boost::random::triangle_distribution<>
#define BOOST_RANDOM_DISTRIBUTION_NAME lognormal
#define BOOST_MATH_DISTRIBUTION boost::math::triangular
#define BOOST_RANDOM_ARG1_TYPE double
#define BOOST_RANDOM_ARG1_NAME b
#define BOOST_RANDOM_ARG1_DEFAULT 0.5
#define BOOST_RANDOM_ARG1_DISTRIBUTION(n) boost::uniform_real<>(0.0001, 0.9999)
#define BOOST_RANDOM_DISTRIBUTION_INIT (0.0, b, 1.0)
#define BOOST_MATH_DISTRIBUTION_INIT (0.0, b, 1.0)

#include "test_real_distribution.ipp"
