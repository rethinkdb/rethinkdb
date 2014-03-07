/* test_geometric.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_geometric.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/geometric_distribution.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/math/distributions/geometric.hpp>
#include <boost/numeric/conversion/cast.hpp>

#define BOOST_RANDOM_DISTRIBUTION boost::random::geometric_distribution<>
#define BOOST_RANDOM_DISTRIBUTION_NAME geometric
#define BOOST_MATH_DISTRIBUTION boost::math::geometric
#define BOOST_RANDOM_ARG1_TYPE double
#define BOOST_RANDOM_ARG1_NAME p
#define BOOST_RANDOM_ARG1_DEFAULT 0.5
#define BOOST_RANDOM_ARG1_DISTRIBUTION(n) boost::uniform_real<>(0.0001, 0.9999)
#define BOOST_RANDOM_DISTRIBUTION_MAX boost::numeric_cast<int>(-5 / std::log(1-p))

#include "test_real_distribution.ipp"
