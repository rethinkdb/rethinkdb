/* test_student_t.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_student_t.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/student_t_distribution.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/math/distributions/students_t.hpp>

#define BOOST_RANDOM_DISTRIBUTION boost::random::student_t_distribution<>
#define BOOST_RANDOM_DISTRIBUTION_NAME student_t
#define BOOST_MATH_DISTRIBUTION boost::math::students_t
#define BOOST_RANDOM_ARG1_TYPE double
#define BOOST_RANDOM_ARG1_NAME n
#define BOOST_RANDOM_ARG1_DEFAULT 1000.0
#define BOOST_RANDOM_ARG1_DISTRIBUTION(n) boost::uniform_real<>(0.00001, n)

#include "test_real_distribution.ipp"
