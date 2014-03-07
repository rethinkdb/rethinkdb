/* test_lagged_fibonacci4423.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_lagged_fibonacci4423.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/lagged_fibonacci.hpp>

#define BOOST_RANDOM_URNG boost::random::lagged_fibonacci4423

#define BOOST_RANDOM_SEED_WORDS 4423*2

#define BOOST_RANDOM_VALIDATION_VALUE 0.23188533286820601
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 0.3872440622693567
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 0.012893676760814543

#define BOOST_RANDOM_GENERATE_VALUES { 0x6D4DBAFU, 0x8039C1A9U, 0x3DA53D58U, 0x95155BE5U }

#include "test_generator.ipp"
