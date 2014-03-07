/* test_lagged_fibonacci3217.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_lagged_fibonacci3217.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/lagged_fibonacci.hpp>

#define BOOST_RANDOM_URNG boost::random::lagged_fibonacci3217

#define BOOST_RANDOM_SEED_WORDS 3217*2

#define BOOST_RANDOM_VALIDATION_VALUE 0.54223093970093927
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 0.073852702370395207
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 0.1805114746514036

#define BOOST_RANDOM_GENERATE_VALUES { 0x4938F127U, 0x86C65CFEU, 0x65356579U, 0xA6CDC325U }

#include "test_generator.ipp"
