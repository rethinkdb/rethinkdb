/* test_mt19937.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_hellekalek1995.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/inversive_congruential.hpp>

#define BOOST_RANDOM_URNG boost::random::hellekalek1995

#define BOOST_RANDOM_SEED_WORDS 1

#define BOOST_RANDOM_VALIDATION_VALUE 1187812169U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 1912573642U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 618743552U

#define BOOST_RANDOM_GENERATE_VALUES { 0x5642A47BU, 0x1F6987E8U, 0xD35860E7U, 0xC8C661ABU }

#include "test_generator.ipp"
