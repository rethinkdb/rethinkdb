/* test_lagged_fibonacci23209.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_lagged_fibonacci23209.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/lagged_fibonacci.hpp>

#define BOOST_RANDOM_URNG boost::random::lagged_fibonacci23209

#define BOOST_RANDOM_SEED_WORDS 23209*2

#define BOOST_RANDOM_VALIDATION_VALUE 0.086299988971202168
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 0.63611281281476195
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 0.0019836425785868528

#define BOOST_RANDOM_GENERATE_VALUES { 0x4301DE0AU, 0xAD2584E3U, 0x7C28463CU, 0x74848542U }

#include "test_generator.ipp"
