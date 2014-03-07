/* test_lagged_fibonacci1279.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_lagged_fibonacci1279.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/lagged_fibonacci.hpp>

#define BOOST_RANDOM_URNG boost::random::lagged_fibonacci1279

#define BOOST_RANDOM_SEED_WORDS 1279*2

#define BOOST_RANDOM_VALIDATION_VALUE 0.39647253381274083
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 0.97108839261370505
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 0.56042480761195179

#define BOOST_RANDOM_GENERATE_VALUES { 0x4D102C47U, 0xC4E610D7U, 0xF29333BEU, 0x6E45EBE7U }

#include "test_generator.ipp"
