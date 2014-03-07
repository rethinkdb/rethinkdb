/* test_lagged_fibonacci19937.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_lagged_fibonacci19937.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/lagged_fibonacci.hpp>

#define BOOST_RANDOM_URNG boost::random::lagged_fibonacci19937

#define BOOST_RANDOM_SEED_WORDS 19937*2

#define BOOST_RANDOM_VALIDATION_VALUE 0.24396310480293693
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 0.95892429604358043
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 0.0029754638678802792

#define BOOST_RANDOM_GENERATE_VALUES { 0x5CE9850CU, 0xAA20067BU, 0x4E48643BU, 0xA4A59F4BU }

#include "test_generator.ipp"
