/* test_uniform_on_sphere_distribution.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_uniform_on_sphere_distribution.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/uniform_on_sphere.hpp>
#include <boost/assign/list_of.hpp>

#include <limits>

#define BOOST_RANDOM_DISTRIBUTION boost::random::uniform_on_sphere<>
#define BOOST_RANDOM_ARG1 dim
#define BOOST_RANDOM_ARG1_DEFAULT 2
#define BOOST_RANDOM_ARG1_VALUE 3

std::vector<double> min0 = boost::assign::list_of(-1.0)(0.0);
std::vector<double> max0 = boost::assign::list_of(1.0)(0.0);
std::vector<double> min1 = boost::assign::list_of(-1.0)(0.0)(0.0);
std::vector<double> max1 = boost::assign::list_of(1.0)(0.0)(0.0);

#define BOOST_RANDOM_DIST0_MIN min0
#define BOOST_RANDOM_DIST0_MAX max0
#define BOOST_RANDOM_DIST1_MIN min1
#define BOOST_RANDOM_DIST1_MAX max1

#define BOOST_RANDOM_TEST1_PARAMS (0)
#define BOOST_RANDOM_TEST1_MIN std::vector<double>()
#define BOOST_RANDOM_TEST1_MAX std::vector<double>()
#define BOOST_RANDOM_TEST2_PARAMS
#define BOOST_RANDOM_TEST2_MIN min0
#define BOOST_RANDOM_TEST2_MAX max0

#include <boost/test/test_tools.hpp>

BOOST_TEST_DONT_PRINT_LOG_VALUE( std::vector<double> )

#include "test_distribution.ipp"
