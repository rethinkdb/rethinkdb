/* test_rand48.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_rand48.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/linear_congruential.hpp>
#include <boost/cstdint.hpp>

#define BOOST_RANDOM_URNG boost::random::rand48

#define BOOST_RANDOM_SEED_WORDS 2

// by experiment from lrand48()
#define BOOST_RANDOM_VALIDATION_VALUE 1993516219U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 1286950069U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 839037874U

#define BOOST_RANDOM_GENERATE_VALUES { 0x55424A4U, 0x3A2CCEF5U, 0x6ADB4A65U, 0x2B019719U }

#include "test_generator.ipp"
