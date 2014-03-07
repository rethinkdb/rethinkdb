/* test_lagged_fibonacci2281.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_lagged_fibonacci2281.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/lagged_fibonacci.hpp>

#define BOOST_RANDOM_URNG boost::random::lagged_fibonacci2281

#define BOOST_RANDOM_SEED_WORDS 2281*2

#define BOOST_RANDOM_VALIDATION_VALUE 0.91955231927349246
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 0.4447517699440553
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 0.087280273457821522

#define BOOST_RANDOM_GENERATE_VALUES { 0x7EB0882AU, 0xCE09BE60U, 0xD53046CFU, 0x93257E41U }

#include "test_generator.ipp"
