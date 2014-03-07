/* test_lagged_fibonacci607.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_lagged_fibonacci607.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/lagged_fibonacci.hpp>

#define BOOST_RANDOM_URNG boost::random::lagged_fibonacci607

#define BOOST_RANDOM_SEED_WORDS 607*2

#define BOOST_RANDOM_VALIDATION_VALUE 0.039230772001715764
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 0.73105942788451372
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 0.72330291632639643

#define BOOST_RANDOM_GENERATE_VALUES { 0x78EB0905U, 0x61766547U, 0xCB507F64U, 0x94FA3EC0U }

#include "test_generator.ipp"
