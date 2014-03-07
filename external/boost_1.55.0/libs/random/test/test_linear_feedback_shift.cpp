/* test_linear_feedback_shift.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_linear_feedback_shift.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/linear_feedback_shift.hpp>

typedef boost::random::linear_feedback_shift_engine<boost::uint32_t, 32, 31, 13, 12> linear_feedback_shift;
#define BOOST_RANDOM_URNG linear_feedback_shift

#define BOOST_RANDOM_SEED_WORDS 1

#define BOOST_RANDOM_VALIDATION_VALUE 981440277U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 3709603036U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 3112279337U

#define BOOST_RANDOM_GENERATE_VALUES { 0x154005U, 0x54005502U, 0x5502BD4U, 0x2BD4005U }

#include "test_generator.ipp"
